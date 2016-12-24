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
#pragma once
#include "TrackerConnection.h"
#include "util/scoped_async_queue.h"
#include <boost/asio/steady_timer.hpp>

namespace librevault {

class FolderGroup;
class P2PProvider;
class PortMappingService;

// BEP-0015 partial implementation (without scrape mechanism)
class UDPTrackerConnection : public TrackerConnection {
public:
	UDPTrackerConnection(url tracker_address, std::shared_ptr<FolderGroup> group_ptr, BTTrackerDiscovery& tracker_discovery, NodeKey& node_key, PortMappingService& port_mapping, io_service& io_service);
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
	using buffer_type = std::shared_ptr<std::vector<char>>;

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

	int32_t gen_transaction_id();

	std::shared_ptr<ScopedAsyncQueue> asio_router_;

	void receive_loop();

	void connect(const boost::system::error_code& ec = boost::system::error_code());
	void announce(const boost::system::error_code& ec = boost::system::error_code());

	void handle_resolve(const boost::system::error_code& ec, udp_resolver::iterator iterator);
	void handle_message(const boost::system::error_code& ec, std::size_t bytes_transferred, std::shared_ptr<udp_endpoint> endpoint_ptr, buffer_type buffer_ptr);

	void handle_connect(buffer_type buffer);
	void handle_announce(buffer_type buffer);
};

} /* namespace librevault */
