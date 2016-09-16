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
#include "util/parse_url.h"
#include "util/Loggable.h"
#include "folder/p2p/WSClient.h"

namespace librevault {

class FolderGroup;
class Client;

class DiscoverySubService : public Loggable {
public:
	DiscoverySubService(Client& client, std::string id);
	virtual ~DiscoverySubService(){}

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