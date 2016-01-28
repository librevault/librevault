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
#include "src/Client.h"
#include "src/folder/FolderGroup.h"
#include "src/folder/p2p/P2PProvider.h"
#include "src/folder/fs/FSFolder.h"
#include "src/folder/p2p/discovery/bttracker/UDPTrackerConnection.h"

namespace librevault {

using namespace boost::asio::ip;

BTTrackerDiscovery::BTTrackerDiscovery(Client& client) :
	DiscoveryService(client) {
	if(client_.config().isBttracker_enabled()) {
		for(auto tracker : client.config().getBttracker_trackers()) {
			trackers_.push_back(tracker);
			log_->debug() << "Added BitTorrent tracker: " << (std::string)tracker;
		}
		client.folder_added_signal.connect(std::bind(&BTTrackerDiscovery::register_group, this, std::placeholders::_1));
		client.folder_removed_signal.connect(std::bind(&BTTrackerDiscovery::unregister_group, this, std::placeholders::_1));
	}else
		log_->info() << "BitTorrent tracker discovery is disabled";
}

BTTrackerDiscovery::~BTTrackerDiscovery() {}

void BTTrackerDiscovery::register_group(std::shared_ptr<FolderGroup> group_ptr) {
	for(auto& tracker_url : trackers_) {
		std::unique_ptr<TrackerConnection> tracker_connection;

		if(tracker_url.scheme == "udp") {
			tracker_connection = std::make_unique<UDPTrackerConnection>(tracker_url, group_ptr, *this, client_);
		}

		groups_.emplace(group_ptr, std::move(tracker_connection));
	}
}

void BTTrackerDiscovery::unregister_group(std::shared_ptr<FolderGroup> group_ptr) {
	groups_.erase(group_ptr);
}

} /* namespace librevault */
