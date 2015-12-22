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
#include "../pch.h"
#pragma once
#include "../util/Loggable.h"
#include "Key.h"
#include "AbstractDirectory.h"
#include "../util/AvailabilityMap.h"

namespace librevault {

class Client;
class Exchanger;

class RemoteDirectory;
class FSDirectory;
class P2PDirectory;

class ExchangeGroup;

class Downloader;

class Downloader : public std::enable_shared_from_this<Downloader>, protected Loggable {
public:
	Downloader(Client& client, ExchangeGroup& exchange_group);

	void put_local_meta(const Meta::PathRevision& revision, bitfield_type bitfield);

	void put_needed_block(const blob& encrypted_data_hash);
	void remove_needed_block(const blob& encrypted_data_hash);

	void handle_choke(std::shared_ptr<RemoteDirectory> remote);
	void handle_unchoke(std::shared_ptr<RemoteDirectory> remote);

	void put_chunk(const blob& encrypted_data_hash, uint32_t offset, const blob& data, std::shared_ptr<RemoteDirectory> from);

private:
	Client& client_;
	ExchangeGroup& exchange_group_;

	/* Needed blocks+request management */
	class NeededBlock {
	public:
		NeededBlock(uint32_t size);
		~NeededBlock();

		uint64_t size() const {return file_map_.size_original();}

		blob get_block();
		void put_chunk(uint32_t offset, const blob& content);

		bool full() const {return file_map_.full();}

		AvailabilityMap::const_iterator begin() {return file_map_.begin();}
		AvailabilityMap::const_iterator end() {return file_map_.end();}

	private:
		AvailabilityMap file_map_;
		fs::path this_block_path_;
		boost::iostreams::mapped_file mapped_file_;
	};

	std::shared_timed_mutex needed_blocks_mtx_;
	std::map<blob, std::shared_ptr<NeededBlock>> needed_blocks_;
	struct ChunkRequest {
		std::shared_ptr<RemoteDirectory> remote;
		std::shared_ptr<NeededBlock> block;
		uint32_t offset;
		uint32_t size;
		std::chrono::steady_clock::time_point started;
	};

	std::mutex requests_mtx_;
	std::multimap<blob, std::shared_ptr<ChunkRequest>> requests_;

	boost::asio::steady_timer maintain_timer_;
	std::mutex maintain_timer_mtx_;
	void maintain_requests(const boost::system::error_code& ec = boost::system::error_code());
	bool request_one();
	std::shared_ptr<RemoteDirectory> find_node_for_request(const blob& encrypted_data_hash);

	void remove_requests_to(std::shared_ptr<RemoteDirectory> remote);

	std::mutex am_unchoked_mtx_;
	std::set<std::shared_ptr<RemoteDirectory>> am_unchoked_;
};

} /* namespace librevault */
