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
#include "UDPLPD.h"
#include "NodeDB.h"
#include "../version.h"
#include <boost/log/trivial.hpp>

#include <memory>
#include <functional>

namespace librevault {

UDPLPD::UDPLPD(boost::asio::io_service& io_service, const boost::property_tree::ptree& properties, NodeDB* node_db) :
		io_service(io_service),
		properties(properties),
		repeat_timer(io_service),
		discovery_socket(io_service),
		node_db(node_db) {
	repeat_interval = std::chrono::seconds(properties.get<uint32_t>("repeat_interval"));
	target_endpoint.port(properties.get<uint16_t>("multicast_port"));
	target_endpoint.address(ip::address::from_string(properties.get<std::string>("multicast_ip")));

	auto bound_endpoint = ip::udp::endpoint(ip::address::from_string(properties.get<std::string>("bind_ip")), properties.get<uint16_t>("bind_port"));
	discovery_socket.open(bound_endpoint.address().is_v6() ? ip::udp::v6() : ip::udp::v4());
	if(bound_endpoint.address().is_v6()){
		discovery_socket.set_option(ip::v6_only(true));
	}

	discovery_socket.set_option(ip::multicast::join_group(target_endpoint.address()));
	discovery_socket.set_option(ip::multicast::enable_loopback(false));
	discovery_socket.set_option(ip::udp::socket::reuse_address(true));

	discovery_socket.bind(bound_endpoint);
}

UDPLPD::~UDPLPD() {
	repeat_timer.cancel();
	discovery_socket.set_option(ip::multicast::leave_group(target_endpoint.address()));
}

void UDPLPD::start() {
	BOOST_LOG_TRIVIAL(debug) << "Started receiving UDP multicasts from: " << target_endpoint;
	receive();
	BOOST_LOG_TRIVIAL(debug) << "Started sending UDP multicasts to: " << target_endpoint;
	send();
}

void UDPLPD::wait_interval() {
	repeat_timer.expires_from_now(repeat_interval);
	repeat_timer.async_wait(std::bind(&UDPLPD::send, this));
}

void UDPLPD::process_rcvd(std::shared_ptr<udp_buffer_t> recv_buffer,
		size_t recv_bytes,
		std::shared_ptr<ip::udp::endpoint> mcast_endpoint_ptr) {

	// Create Protocol Buffer message
	UDPLPD_s message;
	bool parsed_well = message.ParseFromArray(recv_buffer->data(), recv_bytes);

	if(!parsed_well){
		BOOST_LOG_TRIVIAL(warning) << "UDP Multicast from " << *mcast_endpoint_ptr << ": Parse error";
		return;
	}

	ip::udp::endpoint dest_endpoint(mcast_endpoint_ptr->address(), message.port());
	BOOST_LOG_TRIVIAL(debug) << "Local <- " << dest_endpoint;

	node_db->process_UDPLPD_s(dest_endpoint.address(), dest_endpoint.port(), message);

	// We received message, continue receiving others
	receive();
}

void UDPLPD::send() {
	BOOST_LOG_TRIVIAL(debug) << "Local -> " << target_endpoint;
	discovery_socket.async_send_to(buffer(get_multicast_message().SerializeAsString()), target_endpoint, std::bind(&UDPLPD::wait_interval, this));
}

void UDPLPD::receive() {
	auto endpoint = std::make_shared<ip::udp::endpoint>(discovery_socket.local_endpoint());
	auto buffer = std::make_shared<udp_buffer_t>();
	discovery_socket.async_receive_from(boost::asio::buffer(buffer->data(), max_packet_size), *endpoint,
			std::bind(&UDPLPD::process_rcvd, this, buffer, std::placeholders::_2, endpoint));
}

} /* namespace librevault */
