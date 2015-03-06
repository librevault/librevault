/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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

#include "UDPLPD.pb.h"

#include <boost/asio/system_timer.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>

#include <string>
#include <array>

using namespace boost::asio;

namespace librevault {
class NodeDB;

class UDPLPD {
	// Types
	constexpr static size_t max_packet_size = 65536;
	using udp_buffer_t = std::array<uint8_t, max_packet_size>;
	//
	void wait_interval();
	void process_rcvd(std::shared_ptr<udp_buffer_t> recv_buffer,
			size_t recv_bytes,
			std::shared_ptr<ip::udp::endpoint> mcast_endpoint_ptr);

	void send();
	void receive();
protected:
	boost::asio::io_service& io_service;
	const boost::property_tree::ptree& properties;
	NodeDB* node_db;

	UDPLPD_s multicast_message;

	system_timer repeat_timer;
	std::chrono::seconds repeat_interval = std::chrono::seconds(0);

	ip::udp::socket discovery_socket;
	ip::udp::endpoint target_endpoint;
public:
	UDPLPD(boost::asio::io_service& io_service, const boost::property_tree::ptree& properties, NodeDB* node_db);
	virtual ~UDPLPD();

	UDPLPD_s get_multicast_message() const {return multicast_message;}
	void set_multicast_message(const UDPLPD_s message){multicast_message = message;};

	// Processing part
	void start();
};

} /* namespace librevault */
