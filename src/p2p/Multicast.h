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

#include <boost/asio/system_timer.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/trivial.hpp>

#include <string>
#include <array>
#include <functional>

namespace librevault {
namespace p2p {

using namespace boost::asio::ip;
using boost::asio::io_service;
using boost::asio::system_timer;
using boost::property_tree::ptree;

class Multicast {
	mutable std::string message;
protected:
	using udp_buffer = std::array<char, 65536>;

	std::string get_message() const;

	io_service& ios;

	// Options
	ptree& options;

	// Timer
	system_timer repeat_timer;
	std::chrono::seconds repeat_interval = std::chrono::seconds(0);

	// UDP
	udp::socket socket;
	udp::endpoint multicast_addr;
	address bind_address;

	std::function<void(ptree)> handler;

	void wait();
	void process(std::shared_ptr<udp_buffer> buffer, size_t size, std::shared_ptr<udp::endpoint> endpoint_ptr);
	void send();
	void receive();

	Multicast(io_service& ios, ptree& options, decltype(handler) handler);
public:
	virtual ~Multicast();

	void start();
};

class Multicast4 : public Multicast {
public:
	Multicast4(io_service& ios, ptree& options, decltype(handler) handler) : Multicast(ios, options, handler) {
		socket.open(udp::v4());
		bind_address = address::from_string(options.get<std::string>("discovery.multicast4.local_ip"));

		repeat_interval = std::chrono::seconds(options.get<int64_t>("discovery.multicast4.repeat_interval"));
		multicast_addr.port(options.get<uint16_t>("discovery.multicast4.port"));
		multicast_addr.address(address::from_string(options.get<std::string>("discovery.multicast4.ip")));
	}
	virtual ~Multicast4(){}
};

class Multicast6 : public Multicast {
public:
	Multicast6(io_service& ios, ptree& options, decltype(handler) handler) : Multicast(ios, options, handler) {
		socket.open(udp::v6());
		bind_address = address::from_string(options.get<std::string>("discovery.multicast6.local_ip"));

		repeat_interval = std::chrono::seconds(options.get<int64_t>("discovery.multicast6.repeat_interval"));
		multicast_addr.port(options.get<uint16_t>("discovery.multicast6.port"));
		multicast_addr.address(address::from_string(options.get<std::string>("discovery.multicast6.ip")));

		socket.set_option(v6_only(true));
	}
	virtual ~Multicast6(){}
};

} /* namespace discovery */
} /* namespace librevault */
