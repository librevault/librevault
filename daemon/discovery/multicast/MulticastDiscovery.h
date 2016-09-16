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
#include <discovery/DiscoverySubService.h>

namespace librevault {

class MulticastSender;
class MulticastDiscovery : public DiscoverySubService, public std::enable_shared_from_this<MulticastDiscovery> {
	friend class MulticastSender;
public:
	virtual ~MulticastDiscovery();

	void register_group(std::shared_ptr<FolderGroup> group_ptr);
	void unregister_group(std::shared_ptr<FolderGroup> group_ptr);

	virtual void reload_config() = 0;

protected:
	NodeKey& node_key_;

	using udp_buffer = std::array<char, 65536>;

	/* Config parameters */
	bool enabled_;
	udp_endpoint group_;
	std::chrono::seconds repeat_interval_ = std::chrono::seconds(0);

	/* Other members */
	std::map<blob, std::shared_ptr<MulticastSender>> senders_;
	udp_socket socket_;
	address bind_addr_;

	MulticastDiscovery(Client& client, NodeKey& node_key, address bind_addr);

	void start();

	void process(std::shared_ptr<udp_buffer> buffer, size_t size, std::shared_ptr<udp_endpoint> endpoint_ptr, const boost::system::error_code& ec);
	void receive();
};

class MulticastDiscovery4 : public MulticastDiscovery {
public:
	MulticastDiscovery4(Client& client, NodeKey& node_key);
	virtual ~MulticastDiscovery4(){}

	void reload_config() override;
};

class MulticastDiscovery6 : public MulticastDiscovery {
public:
	MulticastDiscovery6(Client& client, NodeKey& node_key);
	virtual ~MulticastDiscovery6(){}

	void reload_config() override;
};

} /* namespace librevault */