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
#include "P2PProvider.h"
#include "P2PFolder.h"
#include <folder/FolderGroup.h>
#include <nat/PortMappingService.h>

#include "WSServer.h"
#include "WSClient.h"

namespace librevault {

P2PProvider::P2PProvider(Client& client, NodeKey& node_key, PortMappingService& port_mapping, FolderService& folder_service) :
		client_(client), node_key_(node_key) {
	ws_server_ = std::make_unique<WSServer>(client, *this, port_mapping, node_key, folder_service);
	ws_client_ = std::make_unique<WSClient>(client, *this, node_key, folder_service);
}

P2PProvider::~P2PProvider() {}

void P2PProvider::add_node(DiscoveryService::ConnectCredentials node_cred, std::shared_ptr<FolderGroup> group_ptr) {
	ws_client_->connect(node_cred, group_ptr);
}

void P2PProvider::mark_loopback(const tcp_endpoint& endpoint) {
	std::unique_lock<std::mutex> lk(loopback_blacklist_mtx_);
	loopback_blacklist_.emplace(endpoint);
	LOGN("Marked " << endpoint << " as loopback");
}

bool P2PProvider::is_loopback(const tcp_endpoint& endpoint) {
	std::unique_lock<std::mutex> lk(loopback_blacklist_mtx_);
	return loopback_blacklist_.find(endpoint) != loopback_blacklist_.end();
}

bool P2PProvider::is_loopback(const blob& pubkey) {
	return node_key_.public_key() == pubkey;
}

} /* namespace librevault */
