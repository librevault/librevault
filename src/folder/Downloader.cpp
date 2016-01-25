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

#include "src/folder/fs/FSFolder.h"
#include "fs/Index.h"
#include "src/folder/p2p/P2PFolder.h"

#include "../Client.h"

namespace librevault {

Downloader::Downloader(Client& client, FolderGroup& exchange_group) :
		Loggable(client, "Downloader"),
		client_(client),
		exchange_group_(exchange_group),
		maintain_timer_(client.network_ios()) {
	log_->trace() << log_tag() << "Downloader()";
	maintain_requests();
}

void Downloader::notify_local_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	log_->trace() << log_tag() << "notify_local_meta()";
	auto smeta = exchange_group_.fs_dir()->get_meta(revision);
	for(size_t block_idx = 0; block_idx < smeta.meta().blocks().size(); block_idx++) {
		if(! bitfield[block_idx]) {
			// We haven't this block, we need to download it
			add_needed_block(smeta.meta().blocks().at(block_idx).encrypted_data_hash_);
		} else {
			// We have have block, remove from needed
			notify_local_block(smeta.meta().blocks().at(block_idx).encrypted_data_hash_);
		}
	}
}

void Downloader::notify_local_block(const blob& encrypted_data_hash) {
	log_->trace() << log_tag() << "notify_local_block()";
	needed_blocks_.erase(encrypted_data_hash);
}

void Downloader::notify_remote_meta(std::shared_ptr<RemoteDirectory> remote, const Meta::PathRevision& revision, bitfield_type bitfield) {
	auto blocks = exchange_group_.fs_dir()->get_meta(revision).meta().blocks();
	for(size_t block_idx = 0; block_idx < blocks.size(); block_idx++)
		if(bitfield[block_idx])
			notify_remote_block(remote, blocks[block_idx].encrypted_data_hash_);
}
void Downloader::notify_remote_block(std::shared_ptr<RemoteDirectory> remote, const blob& encrypted_data_hash) {
	auto needed_block_it = needed_blocks_.find(encrypted_data_hash);
	if(needed_block_it == needed_blocks_.end()) return;

	std::shared_ptr<InterestGuard> guard;
	auto existing_guard_it = interest_guards_.find(remote);
	if(existing_guard_it != interest_guards_.end()) {
		try {
			guard = existing_guard_it->second.lock();
		}catch(std::bad_weak_ptr& e){
			guard = std::make_shared<InterestGuard>(remote);
			existing_guard_it->second = guard;
		}
	}else{
		guard = std::make_shared<InterestGuard>(remote);
		interest_guards_.insert({remote, guard});
	}

	needed_block_it->second->own_block.insert({remote, guard});

	maintain_requests();
}

void Downloader::handle_choke(std::shared_ptr<RemoteDirectory> remote) {
	log_->trace() << log_tag() << "handle_choke()";
	remove_requests_to(remote);
	maintain_requests();
}

void Downloader::handle_unchoke(std::shared_ptr<RemoteDirectory> remote) {
	log_->trace() << log_tag() << "handle_unchoke()";
	maintain_requests();
}

void Downloader::put_chunk(const blob& encrypted_data_hash, uint32_t offset, const blob& data, std::shared_ptr<RemoteDirectory> from) {
	log_->trace() << log_tag() << "put_chunk()";
	auto needed_block_it = needed_blocks_.find(encrypted_data_hash);
	if(needed_block_it == needed_blocks_.end()) return;

	auto& requests = needed_block_it->second->requests;
	for(auto request_it = requests.begin(); request_it != requests.end();) {
		bool incremented_already = false;

		if(request_it->second.offset == offset          // Chunk position incorrect
			&& request_it->second.size == data.size()   // Chunk size incorrect
			&& request_it->first == from) {     // Requested node != replied. Well, it isn't critical, but will be useful to ban "fake" peers

			incremented_already = true;
			request_it = requests.erase(request_it);

			needed_block_it->second->put_chunk(offset, data);
			if(needed_block_it->second->full()) {
				exchange_group_.fs_dir()->put_block(encrypted_data_hash, needed_block_it->second->get_block());
			}   // TODO: catch "invalid hash" exception here

			maintain_requests();
		}

		if(!incremented_already) ++request_it;
	}
}

void Downloader::erase_remote(std::shared_ptr<RemoteDirectory> remote) {
	log_->trace() << log_tag() << "erase_remote()";

	interest_guards_.erase(remote);

	for(auto& needed_block : needed_blocks_) {
		needed_block.second->requests.erase(remote);
		needed_block.second->own_block.erase(remote);
	}
}

void Downloader::remove_requests_to(std::shared_ptr<RemoteDirectory> remote) {
	log_->trace() << log_tag() << "remove_requests_to()";

	for(auto& needed_block : needed_blocks_) {
		needed_block.second->requests.erase(remote);
	}
}

void Downloader::add_needed_block(const blob& encrypted_data_hash) {
	log_->trace() << log_tag() << "add_needed_block()";

	uint32_t unencrypted_blocksize = exchange_group_.fs_dir()->index->get_blocksize(encrypted_data_hash);
	uint32_t padded_blocksize = unencrypted_blocksize % 16 == 0 ? unencrypted_blocksize : ((unencrypted_blocksize/16)+1)*16;
	auto needed_block = std::make_shared<NeededBlock>(padded_blocksize);   // FIXME: This will crash in x32 with OOM because of many mmaped files. Solution: replace mmap with fopen/fwrite/fclose
	needed_blocks_.insert({encrypted_data_hash, needed_block});
}

void Downloader::maintain_requests(const boost::system::error_code& ec) {
	log_->trace() << log_tag() << "maintain_requests()";
	if(ec == boost::asio::error::operation_aborted) return;

	if(maintain_timer_mtx_.try_lock()) {
		std::unique_lock<decltype(maintain_timer_mtx_)> maintain_timer_lk(maintain_timer_mtx_, std::adopt_lock);
		maintain_timer_.cancel();

		// Prune old requests by timeout
		for(auto& needed_block : needed_blocks_) {
			auto& requests = needed_block.second->requests;
			for(auto request = requests.begin(); request != requests.end(); ) {
				if(request->second.started > std::chrono::steady_clock::now() + std::chrono::seconds(10))   // TODO: In config. Very important.
					request = requests.erase(request);
				else
					++request;
			}
		}

		// Make new requests
		for(size_t i = requests_overall(); i <= 10; i++) {    // TODO: This HAS to be in config. 'download_slots' or something.
			bool requested = request_one();
			if(!requested) break;
		}

		maintain_timer_.expires_from_now(std::chrono::seconds(10));  // TODO: Replace with value from config, maybe? We don't like hardcoded values.
		maintain_timer_.async_wait(std::bind(&Downloader::maintain_requests, this, std::placeholders::_1));
	}
}

bool Downloader::request_one() {
	log_->trace() << log_tag() << "request_one()";
	// Try to choose block to request
	for(auto& needed_block : needed_blocks_) {
		auto& encrypted_data_hash = needed_block.first;

		// Try to choose a remote to request this block from
		auto remote = find_node_for_request(encrypted_data_hash);
		if(remote == nullptr) continue;

		// Rebuild request map to determine, which chunk to download now.
		AvailabilityMap<uint32_t> request_map = needed_block.second->file_map();
		for(auto& request : needed_block.second->requests)
			request_map.insert({request.second.offset, request.second.size});

		// Request, actually
		if(!request_map.full()) {
			NeededBlock::ChunkRequest request;
			request.offset = request_map.begin()->first;
			request.size = std::min(request_map.begin()->second, uint32_t(32*1024));    // TODO: Chunk size should be defined in an another place.
			request.started = std::chrono::steady_clock::now();

			remote->request_chunk(encrypted_data_hash, request.offset, request.size);
			needed_block.second->requests.insert({remote, request});
			return true;
		}
	}
	return false;
}

std::shared_ptr<RemoteDirectory> Downloader::find_node_for_request(const blob& encrypted_data_hash) {
	//log_->trace() << log_tag() << "find_node_for_request()";

	auto needed_block_it = needed_blocks_.find(encrypted_data_hash);
	if(needed_block_it == needed_blocks_.end()) return nullptr;

	auto needed_block_ptr = needed_block_it->second;

	for(auto owner_remote : needed_block_ptr->own_block)
		if(! owner_remote.first->peer_choking()) return owner_remote.first; // TODO: implement more smart peer selection algorithm, based on peer weights.

	return nullptr;
}

size_t Downloader::requests_overall() const {
	size_t requests_overall_result = 0;
	for(auto& needed_block : needed_blocks_)
		requests_overall_result += needed_block.second->requests.size();
	return requests_overall_result;
}

Downloader::NeededBlock::NeededBlock(uint32_t size) : file_map_(size) {
	if(BOOST_OS_WINDOWS)
		this_block_path_ = getenv("TEMP");
	else
		this_block_path_ = "/tmp";

	this_block_path_ /= fs::unique_path("librevault-%%%%-%%%%-%%%%-%%%%");

	fs::ofstream os(this_block_path_, std::ios_base::trunc);
	os.close();
	fs::resize_file(this_block_path_, size);
	mapped_file_.open(this_block_path_);
}

Downloader::NeededBlock::~NeededBlock() {
	mapped_file_.close();
	fs::remove(this_block_path_);
}

blob Downloader::NeededBlock::get_block() {
	if(file_map_.size_left() == 0) {
		blob content(size());
		std::copy(mapped_file_.data(), mapped_file_.data() + size(), content.data());
		return content;
	}else throw AbstractFolder::no_such_block();
}

void Downloader::NeededBlock::put_chunk(uint32_t offset, const blob& content) {
	auto inserted = file_map_.insert({offset, content.size()}).second;
	if(inserted) std::copy(content.begin(), content.end(), mapped_file_.data()+offset);
}

} /* namespace librevault */
