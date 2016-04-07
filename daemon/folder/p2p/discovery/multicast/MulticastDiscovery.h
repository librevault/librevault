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
#include "folder/p2p/discovery/DiscoveryService.h"

namespace librevault {

class MulticastSender;
class MulticastDiscovery : public DiscoveryService, public std::enable_shared_from_this<MulticastDiscovery> {
	friend class MulticastSender;
public:
	virtual ~MulticastDiscovery();

	void register_group(std::shared_ptr<FolderGroup> group_ptr);
	void unregister_group(std::shared_ptr<FolderGroup> group_ptr);

	virtual void reload_config() = 0;

protected:
	using udp_buffer = std::array<char, 65536>;

	/* Config parameters */
	bool enabled_;
	udp_endpoint group_;
	std::chrono::seconds repeat_interval_ = std::chrono::seconds(0);

	/* Other members */
	std::map<blob, std::shared_ptr<MulticastSender>> senders_;
	udp_socket socket_;
	address bind_addr_;

	MulticastDiscovery(Client& client, address bind_addr);

	void start();

	void process(std::shared_ptr<udp_buffer> buffer, size_t size, std::shared_ptr<udp_endpoint> endpoint_ptr, const boost::system::error_code& ec);
	void receive();
};

class MulticastDiscovery4 : public MulticastDiscovery {
public:
	MulticastDiscovery4(Client& client);
	virtual ~MulticastDiscovery4(){}

	void reload_config() override;
};

class MulticastDiscovery6 : public MulticastDiscovery {
public:
	MulticastDiscovery6(Client& client);
	virtual ~MulticastDiscovery6(){}

	void reload_config() override;
};

} /* namespace librevault */