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
#include "Multicast.h"
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace librevault {
namespace discovery {

Multicast::Multicast(io_service& ios, ptree& options, decltype(handler) handler) : ios(ios), repeat_timer(ios), socket(ios), options(options), handler(handler) {}
Multicast::~Multicast() {
	socket.set_option(multicast::leave_group(multicast_addr.address()));
}

void Multicast::start(){
	socket.set_option(multicast::join_group(multicast_addr.address()));
	socket.set_option(multicast::enable_loopback(false));
	socket.set_option(udp::socket::reuse_address(true));

	socket.bind(udp::endpoint(bind_address, multicast_addr.port()));

	send();
	receive();

	BOOST_LOG_TRIVIAL(debug) << "Started UDP Local Node Discovery on: " << multicast_addr;
}

std::string Multicast::get_message() const {
	if(message.empty()){
		ptree message_tree;
		message_tree.put("port", options.get<std::string>("net.port"));
		message_tree.put("device_name", options.get<std::string>("device_name", host_name()));
		std::ostringstream os;
		boost::property_tree::json_parser::write_json(os, message_tree, false);
		message = os.str();
	}
	return message;
}

void Multicast::wait(){
	repeat_timer.expires_from_now(repeat_interval);
	repeat_timer.async_wait(std::bind(&Multicast::send, this));
}

void Multicast::process(std::shared_ptr<udp_buffer> buffer, size_t size, std::shared_ptr<udp::endpoint> endpoint_ptr){
	try {
		ptree message_tree;
		std::istringstream is(std::string(buffer->data(), buffer->data()+size));

		boost::property_tree::json_parser::read_json(is, message_tree);
		udp::endpoint dest_endpoint(endpoint_ptr->address(), message_tree.get<uint16_t>("port"));

		BOOST_LOG_TRIVIAL(debug) << "Local <- " << dest_endpoint;
		handler(message_tree);
	}catch(boost::property_tree::ptree_error& e){
		BOOST_LOG_TRIVIAL(debug) << "Message from " << endpoint_ptr->address() << ": Malformed JSON data";
	}
	receive();	// We received message, continue receiving others
}

void Multicast::send(){
	socket.async_send_to(boost::asio::buffer(get_message()), multicast_addr, std::bind(&Multicast::wait, this));
	BOOST_LOG_TRIVIAL(debug) << "Local -> " << multicast_addr;
}

void Multicast::receive(){
	auto endpoint = std::make_shared<udp::endpoint>(socket.local_endpoint());
	auto buffer = std::make_shared<udp_buffer>();
	socket.async_receive_from(boost::asio::buffer(buffer->data(), buffer->size()), *endpoint,
			std::bind(&Multicast::process, this, buffer, std::placeholders::_2, endpoint));
}

} /* namespace discovery */
} /* namespace librevault */
