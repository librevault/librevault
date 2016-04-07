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
#include "P2PProvider.h"
#include "../../Client.h"
#include "P2PFolder.h"
#include "src/folder/FolderGroup.h"
#include "nat/NATPMPService.h"
#include "discovery/StaticDiscovery.h"
#include "src/folder/p2p/discovery/multicast/MulticastDiscovery.h"
#include "src/folder/p2p/discovery/bttracker/BTTrackerDiscovery.h"

#include "WSServer.h"
#include "WSClient.h"

namespace librevault {

P2PProvider::P2PProvider(Client& client) :
		Loggable(client, "P2PProvider"),
		client_(client),
		node_key_(client) {
	ws_server_ = std::make_unique<WSServer>(client, *this);
	ws_client_ = std::make_unique<WSClient>(client, *this);

	static_discovery_ = std::make_unique<StaticDiscovery>(client_);
	multicast4_ = std::make_unique<MulticastDiscovery4>(client_);
	multicast6_ = std::make_unique<MulticastDiscovery6>(client_);
	bttracker_ = std::make_unique<BTTrackerDiscovery>(client_);

	natpmp_ = std::make_unique<NATPMPService>(client_, *this);
	natpmp_->port_signal.connect(std::bind(&P2PProvider::set_public_port, this, std::placeholders::_1));
}

P2PProvider::~P2PProvider() {}

void P2PProvider::mark_loopback(const tcp_endpoint& endpoint) {
	std::unique_lock<std::mutex> lk(loopback_blacklist_mtx_);
	loopback_blacklist_.emplace(endpoint);
	log_->notice() << "Marked " << endpoint << " as loopback";
}

bool P2PProvider::is_loopback(const tcp_endpoint& endpoint) {
	std::unique_lock<std::mutex> lk(loopback_blacklist_mtx_);
	return loopback_blacklist_.find(endpoint) != loopback_blacklist_.end();
}

bool P2PProvider::is_loopback(const blob& pubkey) {
	return node_key().public_key() == pubkey;
}

void P2PProvider::set_public_port(uint16_t port) {
	public_port_ = port ? port : ws_server_->local_endpoint().port();
}

} /* namespace librevault */
