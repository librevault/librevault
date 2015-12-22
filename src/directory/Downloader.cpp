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

#include "ExchangeGroup.h"

#include "fs/FSDirectory.h"
#include "p2p/P2PDirectory.h"

#include "p2p/P2PProvider.h"

#include "Exchanger.h"
#include "../Client.h"

namespace librevault {

Downloader::Downloader(Client& client, ExchangeGroup& exchange_group) :
		Loggable(client, "Downloader"),
		client_(client),
		exchange_group_(exchange_group),
		maintain_timer_(client.network_ios()) {
	log_->trace() << log_tag() << "Downloader()";
	maintain_requests();
}

void Downloader::put_local_meta(const Meta::PathRevision& revision, bitfield_type bitfield) {
	log_->trace() << log_tag() << "put_local_meta()";
	auto smeta = (*exchange_group_.fs_dirs().begin())->get_meta(revision);
	bitfield.resize(smeta.meta().blocks().size());
	for(size_t block_idx = 0; block_idx < smeta.meta().blocks().size(); block_idx++) {
		if(bitfield[block_idx] == false) {
			// We haven't this block, we need to download it
			put_needed_block(smeta.meta().blocks().at(block_idx).encrypted_data_hash_);
		} else {
			// We have have block, remove from needed
			remove_needed_block(smeta.meta().blocks().at(block_idx).encrypted_data_hash_);
		}
	}
}

void Downloader::put_needed_block(const blob& encrypted_data_hash) {
	log_->trace() << log_tag() << "put_needed_block()";
	uint32_t blocksize = (*exchange_group_.fs_dirs().begin())->index->get_blocksize(encrypted_data_hash);
	auto needed_block = std::make_shared<NeededBlock>(blocksize);
	needed_blocks_.insert({encrypted_data_hash, needed_block});
}

void Downloader::remove_needed_block(const blob& encrypted_data_hash) {
	log_->trace() << log_tag() << "remove_needed_block()";
	needed_blocks_.erase(encrypted_data_hash);
	requests_.erase(encrypted_data_hash);
}

void Downloader::handle_choke(std::shared_ptr<RemoteDirectory> remote) {
	log_->trace() << log_tag() << "handle_choke()";
	std::unique_lock<decltype(am_unchoked_mtx_)> lk;
	am_unchoked_.erase(remote);

	remove_requests_to(remote);
}

void Downloader::handle_unchoke(std::shared_ptr<RemoteDirectory> remote) {
	log_->trace() << log_tag() << "handle_unchoke()";
	std::unique_lock<decltype(am_unchoked_mtx_)> lk;
	am_unchoked_.insert(remote);
}

void Downloader::remove_requests_to(std::shared_ptr<RemoteDirectory> remote) {
	log_->trace() << log_tag() << "remove_requests_to()";
	for(auto it = requests_.begin(); it != requests_.end();) {
		if(it->second->remote == remote) {
			it = requests_.erase(it);
		}else{
			it++;
		}
	}
}

void Downloader::put_chunk(const blob& encrypted_data_hash, uint32_t offset, const blob& data, std::shared_ptr<RemoteDirectory> from) {
	log_->trace() << log_tag() << "put_chunk()";
	auto needed_block_it = needed_blocks_.find(encrypted_data_hash);
	if(needed_block_it == needed_blocks_.end()) return;

	auto requests_for_this_block = requests_.equal_range(encrypted_data_hash);
	for(auto request_it = requests_for_this_block.first; request_it != requests_for_this_block.second; request_it++){
		if(request_it->second->offset != offset || request_it->second->size != data.size()) continue;   // Chunk size/position incorrect
		if(request_it->second->remote != from) continue;    // Requested node != replied. Well, it isn't critical, but will be useful to ban "fake" peers

		needed_block_it->second->put_chunk(offset, data);
		if(needed_block_it->second->full()) {
			(*exchange_group_.fs_dirs().begin()).get()->put_block(encrypted_data_hash, needed_block_it->second->get_block());
		}   // TODO: catch "invalid signature" exception here
		requests_.erase(request_it);
		break;
	}
}

void Downloader::maintain_requests(const boost::system::error_code& ec) {
	log_->trace() << log_tag() << "maintain_requests()";
	if(ec == boost::asio::error::operation_aborted) return;

	if(maintain_timer_mtx_.try_lock()) {
		std::unique_lock<decltype(maintain_timer_mtx_)> maintain_timer_lk(maintain_timer_mtx_, std::adopt_lock);
		maintain_timer_.cancel();

		std::unique_lock<decltype(requests_mtx_)> requests_lk(requests_mtx_);
		for(size_t i = requests_.size(); i <= 10; i++) {    // TODO: This HAS to be in config. 'download_slots' or something.
			bool requested = request_one();
			if(!requested) break;
		}

		maintain_timer_.expires_at(std::chrono::steady_clock::now() + std::chrono::seconds(10));  // TODO: Replace with value from config, maybe? We don't like hardcoded values.
		maintain_timer_.async_wait(std::bind(&Downloader::maintain_requests, this, std::placeholders::_1));
	}
}

bool Downloader::request_one() {
	log_->trace() << log_tag() << "request_one()";
	// Try to choose block to request
	for(auto needed_block : needed_blocks_) {
		auto& encrypted_data_hash = needed_block.first;

		// Try to choose a peer to request this block from
		auto node_for_request = find_node_for_request(encrypted_data_hash);
		if(node_for_request == nullptr) continue;

		// Rebuild request map to determine, which chunk to download now.
		AvailabilityMap request_map(needed_block.second->size());
		for(auto request : requests_) {
			if(request.first == needed_block.first) {
				request_map.insert({request.second->offset, request.second->size});
			}
		}
		if(!request_map.full()) {
			// Request, actually
			auto request = std::make_shared<ChunkRequest>();
			request->remote = node_for_request;
			request->block = needed_block.second;
			request->offset = needed_block.second->begin()->first;
			request->size = std::min(needed_block.second->begin()->second, uint64_t(32*1024));
			request->started = std::chrono::steady_clock::now();

			node_for_request->request_chunk(encrypted_data_hash, request->offset, request->size);
			requests_.insert({encrypted_data_hash, request});
			return true;
		}
	}
	return false;
}

std::shared_ptr<RemoteDirectory> Downloader::find_node_for_request(const blob& encrypted_data_hash) {
	//log_->trace() << log_tag() << "find_node_for_request()";

	// Find out Metas, containing this block
	auto containing_metas = (*exchange_group_.fs_dirs().begin())->get_meta_containing(encrypted_data_hash);

	for(auto unchoked_peer : am_unchoked_) {
		for(auto& containing_meta : containing_metas) {
			auto path_id_info_entry = unchoked_peer->path_id_info().at(containing_meta.meta().path_id());
			if(path_id_info_entry.first != containing_meta.meta().revision()) continue;

			auto bitfield = path_id_info_entry.second;
			bitfield.resize(containing_meta.meta().blocks().size());

			for(size_t block_idx = 0; block_idx < path_id_info_entry.second.size(); block_idx++) {
				if(containing_meta.meta().blocks().at(block_idx).encrypted_data_hash_ == encrypted_data_hash && bitfield[block_idx] == true) {
					return unchoked_peer;
				}
			}
		}
	}

	return nullptr;
}

Downloader::NeededBlock::NeededBlock(uint32_t size) : file_map_(size) {
	if(BOOST_OS_WINDOWS)
		this_block_path_ = getenv("TEMP");
	else
		this_block_path_ = "/tmp";

	this_block_path_ /= fs::unique_path();

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
	}else throw AbstractDirectory::no_such_block();
}

void Downloader::NeededBlock::put_chunk(uint32_t offset, const blob& content) {
	auto inserted = file_map_.insert({offset, content.size()}).second;
	if(inserted) std::copy(content.begin(), content.end(), mapped_file_.data()+offset);
}

} /* namespace librevault */
