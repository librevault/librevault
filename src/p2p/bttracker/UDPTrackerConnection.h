/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "../bttracker/TrackerConnection.h"
#include <chrono>

namespace librevault {
namespace p2p {

using namespace boost::asio::ip;

class UDPTrackerConnection: public TrackerConnection {
	/* Types */
	const size_t udb_buffer_max_size = 65535;
	using udp_buffer = std::vector<uint8_t>;

	enum Action : int32_t {
		CONNECT=0, ANNOUNCE=1, SCRAPE=2, ERROR=3, ANNOUNCE6=4
	};
	enum Event : int32_t {
		NONE=0, COMPLETED=1, STARTED=2, STOPPED=3
	};
#pragma pack(push, 1)
	struct conn_req {
		int64_t connection_id;
		int32_t action;
		int32_t transaction_id;
	};
	struct conn_rep {
		int32_t action;
		int32_t transaction_id;
		int64_t connection_id;
	};

	struct announce_req {
		int64_t connection_id;
		int32_t action;
		int32_t transaction_id;
		std::array<uint8_t, 20> info_hash;
		std::array<uint8_t, 20> peer_id;
		int64_t downloaded;
		int64_t left;
		int64_t uploaded;
		int32_t event = NONE;
		uint32_t ip = 0;
		uint32_t key;
		int32_t num_want;
		uint16_t port;
		uint16_t extensions = 0;
	};
	struct announce_rep {
		int32_t action;
		int32_t transaction_id;
		int32_t interval;
		int32_t leechers;
		int32_t seeders;
	};
	struct announce_rep_ext4 {
		int32_t ip4;
		uint16_t port;
	};
	struct announce_rep_ext6 {
		std::array<uint8_t, 16> ip6;
		uint16_t port;
	};
#pragma pack(pop)

	/* Variables */
	address bind_address;

	url tracker_address;

	udp::socket socket;
	udp::resolver resolver;
	udp::endpoint target;

	int64_t connection_id = 0;
	int32_t transaction_id = 0;
	Action action = CONNECT;

	unsigned int fail_count = 0;

	boost::asio::steady_timer reconnect_timer;
	boost::asio::steady_timer response_timer;

	void connect();
	void announce(const infohash& info_hash);
	void receive_loop();
public:
	UDPTrackerConnection(url tracker_address, io_service& ios, Signals& signals, ptree& options);
	virtual ~UDPTrackerConnection();

	virtual void start();
	void handle_resolve(const boost::system::error_code& error, udp::resolver::iterator iterator);
	void handle_connect(std::shared_ptr<udp_buffer> buffer);
	void handle_announce(std::shared_ptr<udp_buffer> buffer);
	void handle_error(std::shared_ptr<udp_buffer> buffer);

	int32_t gen_transaction_id();
};

} /* namespace p2p */
} /* namespace librevault */
