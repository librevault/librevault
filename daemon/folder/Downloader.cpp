/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Downloader.h"

#include "FolderGroup.h"

#include "folder/fs/FSFolder.h"
#include "fs/Index.h"
#include "folder/p2p/P2PFolder.h"

#include "../Client.h"
#include <util/periodic_process.h>

namespace librevault {

Downloader::Downloader(Client& client, FolderGroup& exchange_group) :
		Loggable(client, "Downloader"),
		client_(client),
		exchange_group_(exchange_group),
		periodic_maintain_(client.network_ios(), [this](PeriodicProcess& process){maintain_requests(process);}) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	periodic_maintain_.invoke();
}

Downloader::~Downloader() {
	periodic_maintain_.wait();
}

void Downloader::notify_local_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	auto smeta = exchange_group_.fs_dir()->get_meta(revision);
	for(size_t chunk_idx = 0; chunk_idx < smeta.meta().chunks().size(); chunk_idx++) {
		if(bitfield[chunk_idx]) {
			// We have have block, remove from needed
			notify_local_chunk(smeta.meta().chunks().at(chunk_idx).ct_hash);
		} else {
			// We haven't this block, we need to download it
			add_needed_chunk(smeta.meta().chunks().at(chunk_idx).ct_hash);
		}
	}
}

void Downloader::notify_local_chunk(const blob& ct_hash) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	needed_chunks_.erase(ct_hash);
}

void Downloader::notify_remote_meta(std::shared_ptr<RemoteFolder> remote, const Meta::PathRevision& revision, bitfield_type bitfield) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	try {
		auto blocks = exchange_group_.fs_dir()->get_meta(revision).meta().chunks();
		for(size_t block_idx = 0; block_idx < blocks.size(); block_idx++)
			if(bitfield[block_idx])
				notify_remote_chunk(remote, blocks[block_idx].ct_hash);
	}catch(AbstractFolder::no_such_meta){
		// Well, remote node notifies us about expired meta. It was not requested by us OR another peer sent us newer meta, so this had been expired.
		// Nevertheless, ignore this notification.
	}
}
void Downloader::notify_remote_chunk(std::shared_ptr<RemoteFolder> remote, const blob& ct_hash) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	auto needed_block_it = needed_chunks_.find(ct_hash);
	if(needed_block_it == needed_chunks_.end()) return;

	std::shared_ptr<InterestGuard> guard;
	auto existing_guard_it = interest_guards_.find(remote);
	if(existing_guard_it != interest_guards_.end()) {
		try {
			guard = std::shared_ptr<InterestGuard>(existing_guard_it->second);
		}catch(std::bad_weak_ptr& e){
			guard = std::make_shared<InterestGuard>(remote);
			existing_guard_it->second = guard;
		}
	}else{
		guard = std::make_shared<InterestGuard>(remote);
		interest_guards_.insert({remote, guard});
	}

	needed_block_it->second->own_chunk.insert({remote, guard});

	periodic_maintain_.invoke_post();
}

void Downloader::handle_choke(std::shared_ptr<RemoteFolder> remote) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	remove_requests_to(remote);
	periodic_maintain_.invoke_post();
}

void Downloader::handle_unchoke(std::shared_ptr<RemoteFolder> remote) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	periodic_maintain_.invoke_post();
}

void Downloader::put_block(const blob& ct_hash, uint32_t offset, const blob& data, std::shared_ptr<RemoteFolder> from) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	auto needed_block_it = needed_chunks_.find(ct_hash);
	if(needed_block_it == needed_chunks_.end()) return;

	auto& requests = needed_block_it->second->requests;
	for(auto request_it = requests.begin(); request_it != requests.end();) {
		bool incremented_already = false;

		if(request_it->second.offset == offset          // Chunk position incorrect
			&& request_it->second.size == data.size()   // Chunk size incorrect
			&& request_it->first == from) {     // Requested node != replied. Well, it isn't critical, but will be useful to ban "fake" peers

			incremented_already = true;
			request_it = requests.erase(request_it);

			needed_block_it->second->put_block(offset, data);
			if(needed_block_it->second->full()) {
				exchange_group_.fs_dir()->put_chunk(ct_hash, needed_block_it->second->get_chunk());
			}   // TODO: catch "invalid hash" exception here

			periodic_maintain_.invoke_post();
		}

		if(!incremented_already) ++request_it;
	}
}

void Downloader::erase_remote(std::shared_ptr<RemoteFolder> remote) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	interest_guards_.erase(remote);

	for(auto& needed_block : needed_chunks_) {
		needed_block.second->requests.erase(remote);
		needed_block.second->own_chunk.erase(remote);
	}
}

void Downloader::remove_requests_to(std::shared_ptr<RemoteFolder> remote) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	for(auto& needed_block : needed_chunks_) {
		needed_block.second->requests.erase(remote);
	}
}

void Downloader::add_needed_chunk(const blob& ct_hash) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	uint32_t unencrypted_blocksize = exchange_group_.fs_dir()->index->get_chunk_size(ct_hash);
	uint32_t padded_blocksize = unencrypted_blocksize % 16 == 0 ? unencrypted_blocksize : ((unencrypted_blocksize/16)+1)*16;
	auto needed_block = std::make_shared<NeededChunk>(padded_blocksize);
	needed_chunks_.insert({ct_hash, needed_block});
}

void Downloader::maintain_requests(PeriodicProcess& process) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto request_timeout = std::chrono::seconds(Config::get()->globals()["p2p_request_timeout"].asUInt64());

	// Prune old requests by timeout
	for(auto& needed_block : needed_chunks_) {
		auto& requests = needed_block.second->requests; // We should lock a mutex on this
		for(auto request = requests.begin(); request != requests.end(); ) {
			if(request->second.started + request_timeout < std::chrono::steady_clock::now())
				request = requests.erase(request);
			else
				++request;
		}
	}

	// Make new requests
	for(size_t i = requests_overall(); i < Config::get()->globals()["p2p_download_slots"].asUInt(); i++) {
		bool requested = request_one();
		if(!requested) break;
	}

	process.invoke_after(request_timeout);
}

bool Downloader::request_one() {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	// Try to choose block to request
	for(auto& needed_chunk : needed_chunks_) {
		auto& ct_hash = needed_chunk.first;

		// Try to choose a remote to request this block from
		auto remote = find_node_for_request(ct_hash);
		if(remote == nullptr) continue;

		// Rebuild request map to determine, which block to download now.
		AvailabilityMap<uint32_t> request_map = needed_chunk.second->file_map();
		for(auto& request : needed_chunk.second->requests)
			request_map.insert({request.second.offset, request.second.size});

		// Request, actually
		if(!request_map.full()) {
			NeededChunk::BlockRequest request;
			request.offset = request_map.begin()->first;
			request.size = std::min(request_map.begin()->second, uint32_t(Config::get()->globals()["p2p_block_size"].asUInt()));
			request.started = std::chrono::steady_clock::now();

			remote->request_block(ct_hash, request.offset, request.size);
			needed_chunk.second->requests.insert({remote, request});
			return true;
		}
	}
	return false;
}

std::shared_ptr<RemoteFolder> Downloader::find_node_for_request(const blob& ct_hash) {
	//log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto needed_chunk_it = needed_chunks_.find(ct_hash);
	if(needed_chunk_it == needed_chunks_.end()) return nullptr;

	auto needed_chunk_ptr = needed_chunk_it->second;

	for(auto owner_remote : needed_chunk_ptr->own_chunk)
		if(owner_remote.first->ready() && !owner_remote.first->peer_choking()) return owner_remote.first; // TODO: implement more smart peer selection algorithm, based on peer weights.

	return nullptr;
}

size_t Downloader::requests_overall() const {
	size_t requests_overall_result = 0;
	for(auto& needed_block : needed_chunks_)
		requests_overall_result += needed_block.second->requests.size();
	return requests_overall_result;
}

/* Downloader::NeededChunk */

Downloader::NeededChunk::NeededChunk(uint32_t size) : file_map_(size) {
	if(BOOST_OS_WINDOWS)
		this_block_path_ = getenv("TEMP");
	else
		this_block_path_ = "/tmp";

	this_block_path_ /= fs::unique_path("librevault-%%%%-%%%%-%%%%-%%%%");

	file_wrapper f(this_block_path_, "w");
	f.close();
	fs::resize_file(this_block_path_, size);

#ifndef FOPEN_BACKEND
	mapped_file_.open(this_block_path_);
#else
	wrapped_file_.open(this_block_path_, "w+");
#endif
}

Downloader::NeededChunk::~NeededChunk() {
#ifndef FOPEN_BACKEND
	mapped_file_.close();
#else
	wrapped_file_.close();
#endif
	fs::remove(this_block_path_);
}

blob Downloader::NeededChunk::get_chunk() {
	if(full()) {
		blob content(size());
#ifndef FOPEN_BACKEND
		std::copy(mapped_file_.data(), mapped_file_.data() + size(), content.data());
#else
		wrapped_file_.ios().seekg(0);
		wrapped_file_.ios().read((char*)content.data(), size());
#endif
		return content;
	}else throw AbstractFolder::no_such_chunk();
}

void Downloader::NeededChunk::put_block(uint32_t offset, const blob& content) {
	auto inserted = file_map_.insert({offset, content.size()}).second;
	if(inserted) {
#ifndef FOPEN_BACKEND
		std::copy(content.begin(), content.end(), mapped_file_.data()+offset);
#else
		if(wrapped_file_.ios().tellp() != offset)
			wrapped_file_.ios().seekp(offset);
		wrapped_file_.ios().write((char*)content.data(), content.size());
#endif
	}
}

} /* namespace librevault */
