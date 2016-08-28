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
#include "pch.h"
#include "TrackerConnection.h"

#include <boost/asio/steady_timer.hpp>

namespace librevault {

class FolderGroup;
class P2PProvider;
class Client;

// BEP-0015 partial implementation (without scrape mechanism)
class UDPTrackerConnection : public TrackerConnection {
public:
	UDPTrackerConnection(url tracker_address, std::shared_ptr<FolderGroup> group_ptr, BTTrackerDiscovery& tracker_discovery, Client& client);
	virtual ~UDPTrackerConnection();
private:
	enum class Action : int32_t {
		ACTION_CONNECT=0, ACTION_ANNOUNCE=1, ACTION_SCRAPE=2, ACTION_ERROR=3, ACTION_ANNOUNCE6=4, ACTION_NONE=255
	};

	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("UDPTrackerConnection error") {}
	};

	struct invalid_msg_error : error {
		invalid_msg_error() : error("Invalid message received") {}
	};

#pragma pack(push, 1)
	struct req_header {
		int64_t connection_id_;
		boost::endian::big_int32_t action_;
		int32_t transaction_id_;
	};
	struct rep_header {
		boost::endian::big_int32_t action_;
		int32_t transaction_id_;
	};

	struct conn_req {
		req_header header_;
	};
	struct conn_rep {
		rep_header header_;
		int64_t connection_id_;
	};

	struct announce_req {
		req_header header_;
		btcompat::info_hash info_hash_;
		btcompat::peer_id peer_id_;
		boost::endian::big_int64_t downloaded_;
		boost::endian::big_int64_t left_;
		boost::endian::big_int64_t uploaded_;
		boost::endian::big_int32_t event_ = (int32_t)Event::EVENT_NONE;
		boost::endian::big_uint32_t ip_ = 0;
		int32_t key_;
		boost::endian::big_int32_t num_want_;
		boost::endian::big_uint16_t port_;
		boost::endian::big_uint16_t extensions_ = 0;
	};
	struct announce_rep {
		rep_header header_;
		boost::endian::big_int32_t interval_;
		boost::endian::big_int32_t leechers_;
		boost::endian::big_int32_t seeders_;
	};
#pragma pack(pop)

	const size_t buffer_maxsize_ = 65535;
	std::vector<char> buffer_;
	void reset_buffer();

	address bind_address_;

	udp_socket socket_;
	udp_resolver resolver_;
	udp_endpoint target_;

	boost::asio::steady_timer reconnect_timer_;
	void bump_reconnect_timer();
	boost::asio::steady_timer announce_timer_;
	void bump_announce_timer();
	std::chrono::seconds announce_interval_;

	int64_t connection_id_ = 0;
	int32_t transaction_id_ = 0;
	Action action_ = Action::ACTION_NONE;

	unsigned int fail_count_ = 0;

	int32_t gen_transaction_id();

	void receive_loop();

	void connect(const boost::system::error_code& ec = boost::system::error_code());
	void announce(const boost::system::error_code& ec = boost::system::error_code());

	void handle_resolve(const boost::system::error_code& ec, udp_resolver::iterator iterator);
	void handle_connect();
	void handle_announce();
};

} /* namespace librevault */
