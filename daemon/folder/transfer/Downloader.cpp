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
#include "folder/chunk/ChunkStorage.h"
#include "folder/meta/Index.h"
#include "folder/meta/MetaStorage.h"
#include "util/fs.h"
#include <librevault/crypto/Base32.h>
#include <boost/range/adaptor/map.hpp>

namespace librevault {

/* GlobalFilePool */
std::shared_ptr<file_wrapper> GlobalFilePool::get_file(const boost::filesystem::path& chunk_path) {
	auto file_ptr = cache_[chunk_path].lock();
	if(!file_ptr)
		file_ptr = std::make_shared<file_wrapper>(chunk_path, "r+b");

	release_file(chunk_path);
	retain_file(chunk_path, file_ptr);

	return file_ptr;
}

void GlobalFilePool::release_file(const boost::filesystem::path& system_path) {
	auto it = cache_.find(system_path);
	if(it != cache_.end()) {
		std::shared_ptr<file_wrapper> file_ptr = it->second.lock();
		opened_files_.erase(std::remove(opened_files_.begin(), opened_files_.end(), file_ptr), opened_files_.end());
		cache_.erase(it);
	}
}

void GlobalFilePool::retain_file(boost::filesystem::path chunk_path, std::shared_ptr<file_wrapper> retained_file) {
	cache_[chunk_path] = retained_file;
	opened_files_.push_front(retained_file);
	while(overflow())
		opened_files_.pop_back();
}

/* MissingChunk */
MissingChunk::MissingChunk(const fs::path& system_path, blob ct_hash, uint32_t size) : ct_hash_(std::move(ct_hash)), file_map_(size) {
	this_chunk_path_ = system_path / (std::string("incomplete-") + crypto::Base32().to_string(ct_hash_));

	file_wrapper f(this_chunk_path_, "wb");
	f.close();
	fs::resize_file(this_chunk_path_, size);
}

fs::path MissingChunk::release_chunk() {
	GlobalFilePool::get_instance()->release_file(this_chunk_path_);
	return this_chunk_path_;
}

void MissingChunk::put_block(uint32_t offset, const blob& content) {
	auto inserted = file_map_.insert({offset, content.size()}).second;
	if(inserted) {
		auto file_ptr = GlobalFilePool::get_instance()->get_file(this_chunk_path_);
		if(file_ptr->ios().tellp() != offset)
			file_ptr->ios().seekp(offset);
		file_ptr->ios().write((char*)content.data(), content.size());
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
Downloader::Downloader(const FolderParams& params, MetaStorage* meta_storage, ChunkStorage* chunk_storage, QObject* parent) :
	QObject(parent),
	params_(params),
	meta_storage_(meta_storage),
	chunk_storage_(chunk_storage) {
	LOGFUNC();
	maintain_timer_ = new QTimer(this);
	connect(maintain_timer_, &QTimer::timeout, this, &Downloader::maintain_requests);
	maintain_timer_->setInterval(Config::get()->global_get("p2p_request_timeout").toInt()*1000);
	maintain_timer_->start();
}

Downloader::~Downloader() {}

void Downloader::notify_local_meta(const SignedMeta& smeta, const bitfield_type& bitfield) {
	LOGFUNC();

	bool incomplete_meta = false;

	for(size_t chunk_idx = 0; chunk_idx < smeta.meta().chunks().size(); chunk_idx++) {
		auto& chunk = smeta.meta().chunks().at(chunk_idx);
		auto& ct_hash = chunk.ct_hash;
		if(bitfield[chunk_idx]) {
			// We have chunk, remove from missing
			notify_local_chunk(ct_hash, false); // Do not mark connected chunks as clustered, because they will be marked inside the loop below.
			incomplete_meta = true;
		}else{
			// We haven't this chunk, we need to download it

			/* Compute encrypted chunk size */
			uint32_t padded_chunksize = chunk.size % 16 == 0 ? chunk.size : ((chunk.size / 16) + 1) * 16;

			auto missing_chunk = std::make_shared<MissingChunk>(fs::path(params_.system_path.toStdWString()), ct_hash, padded_chunksize);
			missing_chunks_.insert({ct_hash, missing_chunk});

			/* Add to download queue */
			download_queue_.add_chunk(missing_chunk);
			if(incomplete_meta)
				download_queue_.mark_clustered(missing_chunk);
		}
	}
}

void Downloader::notify_local_chunk(const blob& ct_hash, bool mark_clustered) {
	LOGFUNC();

	// Remove from missing
	auto missing_chunk_it = missing_chunks_.find(ct_hash);
	if(missing_chunk_it != missing_chunks_.end()) {
		download_queue_.remove_chunk(missing_chunk_it->second);
		missing_chunks_.erase(missing_chunk_it);
	}

	// Mark all other chunks "clustered"
	if(mark_clustered) {
		for(auto& smeta : meta_storage_->index->containing_chunk(ct_hash)) {
			for(auto& chunk : smeta.meta().chunks()) {
				auto it = missing_chunks_.find(chunk.ct_hash);
				if(it != missing_chunks_.end())
					download_queue_.mark_clustered(it->second);
			}
		}
	}
}

void Downloader::notify_remote_meta(RemoteFolder* remote, const Meta::PathRevision& revision, bitfield_type bitfield) {
	LOGFUNC();
	try {
		auto chunks = meta_storage_->index->get_meta(revision).meta().chunks();
		for(size_t chunk_idx = 0; chunk_idx < chunks.size(); chunk_idx++)
			if(bitfield[chunk_idx])
				notify_remote_chunk(remote, chunks[chunk_idx].ct_hash);
		remotes_.insert(remote);
		download_queue_.set_overall_remotes_count(remotes_.size());
	}catch(MetaStorage::no_such_meta){
		LOGD("Expired Meta");
		// Well, remote node notifies us about expired meta. It was not requested by us OR another peer sent us newer meta, so this had been expired.
		// Nevertheless, ignore this notification.
	}
}
void Downloader::notify_remote_chunk(RemoteFolder* remote, const blob& ct_hash) {
	LOGFUNC();
	auto missing_chunk_it = missing_chunks_.find(ct_hash);
	if(missing_chunk_it == missing_chunks_.end()) return;

	auto missing_chunk = missing_chunk_it->second;
	missing_chunk->owned_by.insert({remote, remote->get_interest_guard()});
	download_queue_.set_chunk_remotes_count(missing_chunk, missing_chunk->owned_by.size());

	QTimer::singleShot(0, this, &Downloader::maintain_requests);
}

void Downloader::handle_choke(RemoteFolder* remote) {
	LOGFUNC();

	/* Remove requests to this node */
	for(auto& missing_chunk : missing_chunks_)
		missing_chunk.second->requests.erase(remote);

	QTimer::singleShot(0, this, &Downloader::maintain_requests);
}

void Downloader::handle_unchoke(RemoteFolder* remote) {
	LOGFUNC();
	QTimer::singleShot(0, this, &Downloader::maintain_requests);
}

void Downloader::put_block(const blob& ct_hash, uint32_t offset, const blob& data, RemoteFolder* from) {
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
				chunk_storage_->put_chunk(ct_hash, missing_chunk_it->second->release_chunk());
			}   // TODO: catch "invalid hash" exception here

			QTimer::singleShot(0, this, &Downloader::maintain_requests);
		}

		if(!incremented_already) ++request_it;
	}
}

void Downloader::erase_remote(RemoteFolder* remote) {
	LOGFUNC();

	for(auto& missing_chunk : missing_chunks_ | boost::adaptors::map_values) {
		missing_chunk->requests.erase(remote);
		missing_chunk->owned_by.erase(remote);
		download_queue_.set_chunk_remotes_count(missing_chunk, missing_chunk->owned_by.size());
	}
	remotes_.erase(remote);
	download_queue_.set_overall_remotes_count(remotes_.size());
}

void Downloader::maintain_requests() {
	LOGFUNC();

	auto request_timeout = std::chrono::seconds(Config::get()->global_get("p2p_request_timeout").toUInt());

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
	for(size_t i = requests_overall(); i < Config::get()->global_get("p2p_download_slots").toUInt(); i++) {
		bool requested = request_one();
		if(!requested) break;
	}
}

bool Downloader::request_one() {
	LOGFUNC();
	// Try to choose chunk to request
	for(auto missing_chunk : download_queue_.chunks()) {
		// Try to choose a remote to request this block from
		auto remote = find_node_for_request(missing_chunk);
		if(remote == nullptr) continue;

		// Rebuild request map to determine, which block to download now.
		AvailabilityMap<uint32_t> request_map = missing_chunk->file_map();
		for(auto& request : missing_chunk->requests)
			request_map.insert({request.second.offset, request.second.size});

		// Request, actually
		if(!request_map.full()) {
			MissingChunk::BlockRequest request;
			request.offset = request_map.begin()->first;
			request.size = std::min(request_map.begin()->second, uint32_t(Config::get()->global_get("p2p_block_size").toUInt()));
			request.started = std::chrono::steady_clock::now();

			remote->request_block(missing_chunk->ct_hash_, request.offset, request.size);
			missing_chunk->requests.insert({remote, request});
			return true;
		}
	}
	return false;
}

RemoteFolder* Downloader::find_node_for_request(std::shared_ptr<MissingChunk> chunk) {
	//LOGFUNC();

	auto missing_chunk_it = missing_chunks_.find(chunk->ct_hash_);
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
