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
#include "DiscoverySubService.h"
#include "folder/p2p/P2PProvider.h"
#include "folder/p2p/WSClient.h"
#include "Client.h"

namespace librevault {

DiscoverySubService::DiscoverySubService(DiscoveryService& parent, Client& client, std::string id) : Loggable("DiscoverySubService"), parent_(parent), client_(client), id_(id) {}

void DiscoverySubService::add_node(DiscoveryService::ConnectCredentials node_cred, std::shared_ptr<FolderGroup> group_ptr) {
	node_cred.source = id_;
	parent_.discovered_node_signal(node_cred, group_ptr);
}

void DiscoverySubService::add_node(const url& node_url, std::shared_ptr<FolderGroup> group_ptr) {
	DiscoveryService::ConnectCredentials credentials;
	credentials.url = node_url;
	add_node(std::move(credentials), group_ptr);
}

void DiscoverySubService::add_node(const tcp_endpoint& node_endpoint, std::shared_ptr<FolderGroup> group_ptr) {
	DiscoveryService::ConnectCredentials credentials;
	credentials.endpoint = node_endpoint;
	add_node(std::move(credentials), group_ptr);
}

void DiscoverySubService::add_node(const tcp_endpoint& node_endpoint, const blob& pubkey, std::shared_ptr<FolderGroup> group_ptr) {
	DiscoveryService::ConnectCredentials credentials;
	credentials.endpoint = node_endpoint;
	credentials.pubkey = pubkey;
	add_node(std::move(credentials), group_ptr);
}

} /* namespace librevault */
