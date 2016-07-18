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
#pragma once
#include <util/file_util.h>
#include <util/periodic_process.h>
#include <boost/bimap.hpp>
#include "RemoteFolder.h"
#include "util/AvailabilityMap.h"

#define CLUSTERED_COEFFICIENT 10
#define IMMEDIATE_COEFFICIENT 20
#define RARITY_COEFFICIENT 25

namespace librevault {

class Client;
class FolderGroup;

class Downloader : public std::enable_shared_from_this<Downloader>, protected Loggable {
public:
	Downloader(Client& client, FolderGroup& exchange_group);
	~Downloader();

	void notify_local_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield);
	void notify_local_chunk(const blob& ct_hash);

	void notify_remote_meta(std::shared_ptr<RemoteFolder> remote, const Meta::PathRevision& revision, bitfield_type bitfield);
	void notify_remote_chunk(std::shared_ptr<RemoteFolder> remote, const blob& ct_hash);

	void handle_choke(std::shared_ptr<RemoteFolder> remote);
	void handle_unchoke(std::shared_ptr<RemoteFolder> remote);

	void put_block(const blob& ct_hash, uint32_t offset, const blob& data, std::shared_ptr<RemoteFolder> from);

	void erase_remote(std::shared_ptr<RemoteFolder> remote);

private:
	Client& client_;
	FolderGroup& exchange_group_;

	/* Missing chunk+request management */
	struct MissingChunk {
		MissingChunk(blob ct_hash, uint32_t size);
		~MissingChunk();

		blob get_chunk();
		void put_block(uint32_t offset, const blob& content);

		// size-related functions
		uint64_t size() const {return file_map_.size_original();}
		bool complete() const {return file_map_.full();}

		// AvailabilityMap accessors
		AvailabilityMap<uint32_t>::const_iterator begin() {return file_map_.begin();}
		AvailabilityMap<uint32_t>::const_iterator end() {return file_map_.end();}
		const AvailabilityMap<uint32_t>& file_map() const {return file_map_;}

		/* Request-oriented functions */
		struct BlockRequest {
			uint32_t offset;
			uint32_t size;
			std::chrono::steady_clock::time_point started;
		};
		std::multimap<std::shared_ptr<RemoteFolder>, BlockRequest> requests;
		std::map<std::shared_ptr<RemoteFolder>, std::shared_ptr<RemoteFolder::InterestGuard>> owned_by;

		using weight_t = float;
		weight_t weight_;

		const blob ct_hash_;

	private:
		AvailabilityMap<uint32_t> file_map_;
		fs::path this_chunk_path_;
#ifndef FOPEN_BACKEND
		boost::iostreams::mapped_file mapped_file_;
#else
		file_wrapper wrapped_file_;
#endif
	};

	std::map<blob, std::shared_ptr<MissingChunk>> missing_chunks_;
	struct weight_less {
		inline bool operator() (std::shared_ptr<MissingChunk> a, std::shared_ptr<MissingChunk> b) const {
			return a->weight_ < b->weight_;
		}
	};
	using weight_ordered_chunks_t = boost::bimap<
		std::shared_ptr<MissingChunk>,
		std::shared_ptr<MissingChunk>
	>;
	weight_ordered_chunks_t weight_ordered_chunks_;
	void reweight_chunk(const blob& ct_hash);

	size_t requests_overall() const;

	PeriodicProcess periodic_maintain_;
	void maintain_requests(PeriodicProcess& process);

	bool request_one();
	std::shared_ptr<RemoteFolder> find_node_for_request(const blob& ct_hash);

	void add_missing_chunk(const blob& ct_hash);
	void remove_requests_to(std::shared_ptr<RemoteFolder> remote);

	/* Node management */
	std::set<std::shared_ptr<RemoteFolder>> remotes_;
};

} /* namespace librevault */
