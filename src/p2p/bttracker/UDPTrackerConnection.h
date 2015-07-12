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
#include "../../Session.h"
#include <boost/endian/arithmetic.hpp>
#include <chrono>

namespace librevault {
namespace p2p {

using namespace boost::asio::ip;

class UDPTrackerConnection: public TrackerConnection {
	/* Types */
	const size_t udp_buffer_max_size = 65535;
	using udp_buffer = std::vector<uint8_t>;

	enum class Action : int32_t {
		CONNECT=0, ANNOUNCE=1, SCRAPE=2, ERROR=3, ANNOUNCE6=4, NONE=255
	};
	enum class Event : int32_t {
		NONE=0, COMPLETED=1, STARTED=2, STOPPED=3
	};
#pragma pack(push, 1)
	struct req_header {
		int64_t connection_id;
		boost::endian::big_int32_t action;
		int32_t transaction_id;
	};
	struct rep_header {
		boost::endian::big_int32_t action;
		int32_t transaction_id;
	};

	struct conn_req {
		req_header header;
	};
	struct conn_rep {
		rep_header header;
		int64_t connection_id;
	};

	struct announce_req {
		req_header header;
		std::array<uint8_t, 20> info_hash;
		std::array<uint8_t, 20> peer_id;
		boost::endian::big_int64_t downloaded;
		boost::endian::big_int64_t left;
		boost::endian::big_int64_t uploaded;
		boost::endian::big_int32_t event = (int32_t)Event::NONE;
		boost::endian::big_uint32_t ip = 0;
		uint32_t key;
		boost::endian::big_int32_t num_want;
		boost::endian::big_uint16_t port;
		boost::endian::big_uint16_t extensions = 0;
	};
	struct announce_rep {
		rep_header header;
		boost::endian::big_int32_t interval;
		boost::endian::big_int32_t leechers;
		boost::endian::big_int32_t seeders;
	};
	struct announce_rep_ext4 {
		std::array<uint8_t, 4> ip4;
		boost::endian::big_uint16_t port;
	};
	struct announce_rep_ext6 {
		std::array<uint8_t, 16> ip6;
		boost::endian::big_uint16_t port;
	};
#pragma pack(pop)

	/* Variables */
	address bind_address;

	udp::socket socket;
	udp::resolver resolver;
	udp::endpoint target;

	boost::asio::steady_timer timer;
	time_point connected_time;
	void start_timer();

	int64_t connection_id = 0;
	int32_t transaction_id = 0;
	Action action = Action::NONE;

	unsigned int fail_count = 0;

	void handle_resolve(const boost::system::error_code& error, udp::resolver::iterator iterator);
	void receive_loop();

	void connect(const boost::system::error_code& error = boost::system::error_code());
	void announce(const boost::system::error_code& error = boost::system::error_code());
public:
	UDPTrackerConnection(url tracker_address, Session& session);
	virtual ~UDPTrackerConnection();

	void handle_connect(std::shared_ptr<udp_buffer> buffer);
	void handle_announce(std::shared_ptr<udp_buffer> buffer);
	void handle_error(std::shared_ptr<udp_buffer> buffer);

	int32_t gen_transaction_id();
};

} /* namespace p2p */
} /* namespace librevault */
