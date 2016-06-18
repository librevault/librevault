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
#include "util/parse_url.h"
#include "util/Loggable.h"
#include "folder/p2p/WSClient.h"

namespace librevault {

class FolderGroup;
class Client;

class DiscoveryService : public Loggable {
public:
	DiscoveryService(Client& client, std::string id);
	virtual ~DiscoveryService(){}

	virtual void register_group(std::shared_ptr<FolderGroup> group_ptr) = 0;
	virtual void unregister_group(std::shared_ptr<FolderGroup> group_ptr) = 0;

	void add_node(WSClient::ConnectCredentials node_cred, std::shared_ptr<FolderGroup> group_ptr);
	void add_node(const url& node_url, std::shared_ptr<FolderGroup> group_ptr);
	void add_node(const tcp_endpoint& node_endpoint, std::shared_ptr<FolderGroup> group_ptr);
	void add_node(const tcp_endpoint& node_endpoint, const blob& pubkey, std::shared_ptr<FolderGroup> group_ptr);

	Client& client() {return client_;}

protected:
	Client& client_;
	std::string id_;
};

} /* namespace librevault */