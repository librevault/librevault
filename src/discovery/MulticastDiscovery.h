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
#include "../pch.h"
#include "DiscoveryService.h"

namespace librevault {

class MulticastDiscovery;

class MulticastSender {
public:
	MulticastSender(MulticastDiscovery& parent, std::shared_ptr<ExchangeGroup> exchange_group);
	std::string get_message() const;

	void wait();
	void send();

private:
	MulticastDiscovery& parent_;
	std::shared_ptr<ExchangeGroup> exchange_group_;

	boost::asio::system_timer repeat_timer_;
	std::chrono::seconds& repeat_interval_;

	mutable std::string message_;
};

class MulticastDiscovery : public DiscoveryService {
	friend class MulticastSender;
public:
	virtual ~MulticastDiscovery();

	void register_group(std::shared_ptr<ExchangeGroup> group_ptr);
	void unregister_group(std::shared_ptr<ExchangeGroup> group_ptr);
protected:
	using udp_buffer = std::array<char, 65536>;

	std::map<std::shared_ptr<ExchangeGroup>, std::shared_ptr<MulticastSender>> senders_;

	std::chrono::seconds repeat_interval_ = std::chrono::seconds(0);

	// Options
	const ptree& local_options_;

	// UDP
	udp_socket socket_;
	udp_endpoint multicast_addr_;
	address bind_address_;

	void start();

	void process(std::shared_ptr<udp_buffer> buffer, size_t size, std::shared_ptr<udp_endpoint> endpoint_ptr);
	void receive();

	MulticastDiscovery(Client& client, Exchanger& exchanger, const ptree& options);
};

class MulticastDiscovery4 : public MulticastDiscovery {
public:
	MulticastDiscovery4(Client& client, Exchanger& exchanger);
	virtual ~MulticastDiscovery4(){}

	std::string log_tag() const {return "[MulticastDiscovery4] ";}
};

class MulticastDiscovery6 : public MulticastDiscovery {
public:
	MulticastDiscovery6(Client& client, Exchanger& exchanger);
	virtual ~MulticastDiscovery6(){}

	std::string log_tag() const {return "[MulticastDiscovery6] ";}
};

} /* namespace librevault */