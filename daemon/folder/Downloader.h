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
#include "RemoteFolder.h"
#include "util/AvailabilityMap.h"

namespace librevault {

class Client;
class FolderGroup;

class Downloader : public std::enable_shared_from_this<Downloader>, protected Loggable {
public:
	Downloader(Client& client, FolderGroup& exchange_group);

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

	/* RAII wrapper for sending INTERESTED/NOT_INTERESTED messages. Should be used with reference counter */
	struct InterestGuard {
		InterestGuard(std::shared_ptr<RemoteFolder> remote) : remote_(remote) {remote_->interest();}
		~InterestGuard() {remote_->uninterest();}
	private:
		std::shared_ptr<RemoteFolder> remote_;
	};
	std::map<std::shared_ptr<RemoteFolder>, std::weak_ptr<InterestGuard>> interest_guards_;

	/* Needed blocks+request management */
	struct NeededChunk {
		NeededChunk(uint32_t size);
		~NeededChunk();

		blob get_chunk();
		void put_block(uint32_t offset, const blob& content);

		// size-related functions
		uint64_t size() const {return file_map_.size_original();}
		bool full() const {return file_map_.full();}

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
		std::map<std::shared_ptr<RemoteFolder>, std::shared_ptr<InterestGuard>> own_chunk;

	private:
		AvailabilityMap<uint32_t> file_map_;
		fs::path this_block_path_;
#ifndef FOPEN_BACKEND
		boost::iostreams::mapped_file mapped_file_;
#else
		file_wrapper wrapped_file_;
#endif
	};

	std::map<blob, std::shared_ptr<NeededChunk>> needed_chunks_;
	size_t requests_overall() const;

	boost::asio::steady_timer maintain_timer_;
	std::mutex maintain_timer_mtx_;
	void maintain_requests(const boost::system::error_code& ec = boost::system::error_code());
	bool request_one();
	std::shared_ptr<RemoteFolder> find_node_for_request(const blob& ct_hash);

	void add_needed_chunk(const blob& ct_hash);
	void remove_requests_to(std::shared_ptr<RemoteFolder> remote);
};

} /* namespace librevault */
