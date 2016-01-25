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
#include "BTTrackerDiscovery.h"
#include "../Client.h"
#include "../directory/ExchangeGroup.h"
#include "../directory/p2p/P2PProvider.h"
#include "../directory/fs/FSDirectory.h"
#include "bttracker/UDPTrackerConnection.h"

namespace librevault {

using namespace boost::asio::ip;

BTTrackerDiscovery::BTTrackerDiscovery(Client& client) :
	DiscoveryService(client) {
	if(client_.config().current.discovery_bttracker_enabled) {
		for(auto tracker : client.config().current.discovery_bttracker_trackers) {
			trackers_.push_back(tracker);
			log_->debug() << "Added BitTorrent tracker: " << (std::string)tracker;
		}
	}else
		log_->info() << "BitTorrent tracker discovery is disabled";
}

BTTrackerDiscovery::~BTTrackerDiscovery() {}

void BTTrackerDiscovery::register_group(std::shared_ptr<ExchangeGroup> group_ptr) {
	for(auto& tracker_url : trackers_) {
		std::unique_ptr<TrackerConnection> tracker_connection;

		if(tracker_url.scheme == "udp") {
			tracker_connection = std::make_unique<UDPTrackerConnection>(tracker_url, group_ptr, *this, client_);
		}

		groups_.emplace(group_ptr, std::move(tracker_connection));
	}
}

void BTTrackerDiscovery::unregister_group(std::shared_ptr<ExchangeGroup> group_ptr) {
	groups_.erase(group_ptr);
}

} /* namespace librevault */
