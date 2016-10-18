/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "Downloader.h"

#include "control/Config.h"
#include "control/FolderParams.h"
#include "folder/AbstractFolder.h"
#include "folder/chunk/ChunkStorage.h"
#include "folder/meta/Index.h"
#include "folder/meta/MetaStorage.h"
#include "util/fs.h"
#include <librevault/crypto/Base32.h>
#include <boost/range/adaptor/map.hpp>

namespace librevault {

/* MissingChunk */
MissingChunk::MissingChunk(const fs::path& system_path, blob ct_hash, uint32_t size) : ct_hash_(std::move(ct_hash)), file_map_(size) {
	this_chunk_path_ = system_path / (std::string("incomplete-") + crypto::Base32().to_string(ct_hash_));

	file_wrapper f(this_chunk_path_, "wb");
	f.close();
	fs::resize_file(this_chunk_path_, size);

#ifndef FOPEN_BACKEND
	mapped_file_.open(this_chunk_path_);
#else
	wrapped_file_.open(this_chunk_path_, "r+b");
#endif
}

fs::path MissingChunk::release_chunk() {
#ifndef FOPEN_BACKEND
	mapped_file_.close();
#else
	wrapped_file_.close();
#endif
	return this_chunk_path_;
}

void MissingChunk::put_block(uint32_t offset, const blob& content) {
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

/* WeightedDownloadQueue */
float WeightedDownloadQueue::Weight::value() const {
	float weight_value = 0;

	weight_value += CLUSTERED_COEFFICIENT * (clustered ? 1 : 0);
	weight_value += IMMEDIATE_COEFFICIENT * (immediate ? 1 : 0);
	float rarity = (float)(remotes_count - owned_by) / (float)remotes_count;
	weight_value += rarity * RARITY_COEFFICIENT;

	return weight_value;
}

WeightedDownloadQueue::Weight WeightedDownloadQueue::get_current_weight(std::shared_ptr<MissingChunk> chunk) {
	auto it = weight_ordered_chunks_.left.find(chunk);
	if(it != weight_ordered_chunks_.left.end())
		return it->second;
	else
		return Weight();
}

void WeightedDownloadQueue::reweight_chunk(std::shared_ptr<MissingChunk> chunk, Weight new_weight) {
	auto chunk_it = weight_ordered_chunks_.left.find(chunk);

	if(chunk_it != weight_ordered_chunks_.left.end() && chunk_it->second != new_weight) {
		weight_ordered_chunks_.left.erase(chunk_it);
		weight_ordered_chunks_.left.insert(queue_left_value(chunk, new_weight));
	}
}

void WeightedDownloadQueue::add_chunk(std::shared_ptr<MissingChunk> chunk) {
	weight_ordered_chunks_.left.insert(queue_left_value(chunk, Weight()));
}

void WeightedDownloadQueue::remove_chunk(std::shared_ptr<MissingChunk> chunk) {
	weight_ordered_chunks_.left.erase(chunk);
}

void WeightedDownloadQueue::set_overall_remotes_count(size_t count) {
	weight_ordered_chunks_t new_queue;
	for(auto& entry : weight_ordered_chunks_.left) {
		std::shared_ptr<MissingChunk> chunk = entry.first;
		Weight weight = entry.second;

		weight.remotes_count = count;
		new_queue.left.insert(queue_left_value(chunk, weight));
	}
	weight_ordered_chunks_ = new_queue;
}

void WeightedDownloadQueue::set_chunk_remotes_count(std::shared_ptr<MissingChunk> chunk, size_t count) {
	Weight weight = get_current_weight(chunk);
	weight.remotes_count = count;

	reweight_chunk(chunk, weight);
}

void WeightedDownloadQueue::mark_clustered(std::shared_ptr<MissingChunk> chunk) {
	Weight weight = get_current_weight(chunk);
	weight.clustered = true;

	reweight_chunk(chunk, weight);
}

void WeightedDownloadQueue::mark_immediate(std::shared_ptr<MissingChunk> chunk) {
	Weight weight = get_current_weight(chunk);
	weight.immediate = true;

	reweight_chunk(chunk, weight);
}

std::list<std::shared_ptr<MissingChunk>> WeightedDownloadQueue::chunks() const {
	std::list<std::shared_ptr<MissingChunk>> chunk_list;
	for(auto& chunk_ptr : boost::adaptors::values(weight_ordered_chunks_.right))
		chunk_list.push_back(chunk_ptr);
	return chunk_list;
}

/* Downloader */
Downloader::Downloader(const FolderParams& params, MetaStorage& meta_storage, ChunkStorage& chunk_storage, io_service& ios) :
	params_(params), meta_storage_(meta_storage), chunk_storage_(chunk_storage),
	periodic_maintain_(ios, [this](PeriodicProcess& process){maintain_requests(process);}) {
	LOGFUNC();
	periodic_maintain_.invoke();
}

Downloader::~Downloader() {
	periodic_maintain_.wait();
}

void Downloader::notify_local_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	LOGFUNC();

	std::set<blob> missing_ct_hashes;

	bool incomplete_meta = false;

	auto smeta = meta_storage_.index->get_meta(revision);
	for(size_t chunk_idx = 0; chunk_idx < smeta.meta().chunks().size(); chunk_idx++) {
		auto& ct_hash = smeta.meta().chunks().at(chunk_idx).ct_hash;
		if(bitfield[chunk_idx]) {
			// We have chunk, remove from missing
			notify_local_chunk(ct_hash);
			incomplete_meta = true;
		} else {
			// We haven't this chunk, we need to download it
			missing_ct_hashes.insert(ct_hash);
		}
	}

	for(auto& ct_hash : missing_ct_hashes) {
		/* Compute encrypted chunk size */
		uint32_t pt_chunksize = meta_storage_.index->get_chunk_size(ct_hash);
		uint32_t padded_chunksize = pt_chunksize % 16 == 0 ? pt_chunksize : ((pt_chunksize / 16) + 1) * 16;

		auto missing_chunk = std::make_shared<MissingChunk>(params_.system_path, ct_hash, padded_chunksize);
		missing_chunks_.insert({ct_hash, missing_chunk});

		/* Add to download queue */
		download_queue_.add_chunk(missing_chunk);
		if(incomplete_meta)
			download_queue_.mark_clustered(missing_chunk);
	}
}

void Downloader::notify_local_chunk(const blob& ct_hash) {
	LOGFUNC();

	// Remove from missing
	auto missing_chunk_it = missing_chunks_.find(ct_hash);
	if(missing_chunk_it != missing_chunks_.end()) {
		download_queue_.remove_chunk(missing_chunk_it->second);
		missing_chunks_.erase(missing_chunk_it);
	}

	// Mark all other chunks "clustered"
	for(auto& smeta : meta_storage_.index->containing_chunk(ct_hash))
		for(auto& chunk : smeta.meta().chunks()) {
			auto it = missing_chunks_.find(chunk.ct_hash);
			if(it != missing_chunks_.end())
				download_queue_.mark_clustered(it->second);
		}
}

void Downloader::notify_remote_meta(std::shared_ptr<RemoteFolder> remote, const Meta::PathRevision& revision, bitfield_type bitfield) {
	LOGFUNC();
	try {
		auto chunks = meta_storage_.index->get_meta(revision).meta().chunks();
		for(size_t chunk_idx = 0; chunk_idx < chunks.size(); chunk_idx++)
			if(bitfield[chunk_idx])
				notify_remote_chunk(remote, chunks[chunk_idx].ct_hash);
		remotes_.insert(remote);
		download_queue_.set_overall_remotes_count(remotes_.size());
	}catch(AbstractFolder::no_such_meta){
		// Well, remote node notifies us about expired meta. It was not requested by us OR another peer sent us newer meta, so this had been expired.
		// Nevertheless, ignore this notification.
	}
}
void Downloader::notify_remote_chunk(std::shared_ptr<RemoteFolder> remote, const blob& ct_hash) {
	LOGFUNC();
	auto missing_chunk_it = missing_chunks_.find(ct_hash);
	if(missing_chunk_it == missing_chunks_.end()) return;

	auto missing_chunk = missing_chunk_it->second;
	missing_chunk->owned_by.insert({remote, remote->get_interest_guard()});
	download_queue_.set_chunk_remotes_count(missing_chunk, missing_chunk->owned_by.size());

	periodic_maintain_.invoke_post();
}

void Downloader::handle_choke(std::shared_ptr<RemoteFolder> remote) {
	LOGFUNC();

	/* Remove requests to this node */
	for(auto& missing_chunk : missing_chunks_)
		missing_chunk.second->requests.erase(remote);

	periodic_maintain_.invoke_post();
}

void Downloader::handle_unchoke(std::shared_ptr<RemoteFolder> remote) {
	LOGFUNC();
	periodic_maintain_.invoke_post();
}

void Downloader::put_block(const blob& ct_hash, uint32_t offset, const blob& data, std::shared_ptr<RemoteFolder> from) {
	LOGFUNC();
	auto missing_chunk_it = missing_chunks_.find(ct_hash);
	if(missing_chunk_it == missing_chunks_.end()) return;

	auto& requests = missing_chunk_it->second->requests;
	for(auto request_it = requests.begin(); request_it != requests.end();) {
		bool incremented_already = false;

		if(request_it->second.offset == offset          // Chunk position incorrect
			&& request_it->second.size == data.size()   // Chunk size incorrect
			&& request_it->first == from) {     // Requested node != replied. Well, it isn't critical, but will be useful to ban "fake" peers

			incremented_already = true;
			request_it = requests.erase(request_it);

			missing_chunk_it->second->put_block(offset, data);
			if(missing_chunk_it->second->complete()) {
				chunk_storage_.put_chunk(ct_hash, missing_chunk_it->second->release_chunk());
			}   // TODO: catch "invalid hash" exception here

			periodic_maintain_.invoke_post();
		}

		if(!incremented_already) ++request_it;
	}
}

void Downloader::erase_remote(std::shared_ptr<RemoteFolder> remote) {
	LOGFUNC();

	for(auto& missing_chunk : missing_chunks_ | boost::adaptors::map_values) {
		missing_chunk->requests.erase(remote);
		missing_chunk->owned_by.erase(remote);
		download_queue_.set_chunk_remotes_count(missing_chunk, missing_chunk->owned_by.size());
	}
	remotes_.erase(remote);
	download_queue_.set_overall_remotes_count(remotes_.size());
}

void Downloader::maintain_requests(PeriodicProcess& process) {
	LOGFUNC();

	auto request_timeout = std::chrono::seconds(Config::get()->globals()["p2p_request_timeout"].asUInt64());

	// Prune old requests by timeout
	for(auto& missing_chunk : missing_chunks_) {
		auto& requests = missing_chunk.second->requests; // We should lock a mutex on this
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
	LOGFUNC();
	// Try to choose chunk to request
	for(auto missing_chunk : download_queue_.chunks()) {
		auto& ct_hash = missing_chunk->ct_hash_;

		// Try to choose a remote to request this block from
		auto remote = find_node_for_request(ct_hash);
		if(remote == nullptr) continue;

		// Rebuild request map to determine, which block to download now.
		AvailabilityMap<uint32_t> request_map = missing_chunk->file_map();
		for(auto& request : missing_chunk->requests)
			request_map.insert({request.second.offset, request.second.size});

		// Request, actually
		if(!request_map.full()) {
			MissingChunk::BlockRequest request;
			request.offset = request_map.begin()->first;
			request.size = std::min(request_map.begin()->second, uint32_t(Config::get()->globals()["p2p_block_size"].asUInt()));
			request.started = std::chrono::steady_clock::now();

			remote->request_block(ct_hash, request.offset, request.size);
			missing_chunk->requests.insert({remote, request});
			return true;
		}
	}
	return false;
}

std::shared_ptr<RemoteFolder> Downloader::find_node_for_request(const blob& ct_hash) {
	//LOGFUNC();

	auto missing_chunk_it = missing_chunks_.find(ct_hash);
	if(missing_chunk_it == missing_chunks_.end()) return nullptr;

	auto missing_chunk_ptr = missing_chunk_it->second;

	for(auto owner_remote : missing_chunk_ptr->owned_by)
		if(owner_remote.first->ready() && !owner_remote.first->peer_choking()) return owner_remote.first; // TODO: implement more smart peer selection algorithm, based on peer weights.

	return nullptr;
}

size_t Downloader::requests_overall() const {
	size_t requests_overall_result = 0;
	for(auto& missing_chunk : missing_chunks_)
		requests_overall_result += missing_chunk.second->requests.size();
	return requests_overall_result;
}

} /* namespace librevault */
