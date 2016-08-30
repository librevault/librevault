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
#include "BTTrackerDiscovery.h"
#include "Client.h"
#include "folder/FolderGroup.h"
#include "folder/p2p/P2PProvider.h"
#include "folder/fs/FSFolder.h"
#include "folder/p2p/discovery/bttracker/UDPTrackerConnection.h"

namespace librevault {

using namespace boost::asio::ip;

BTTrackerDiscovery::BTTrackerDiscovery(Client& client) :
	DiscoveryService(client, "BT") {
	if(Config::get()->globals()["bttracker_enabled"].asBool()) {
		for(auto tracker : Config::get()->globals()["bttracker_trackers"]) {
			trackers_.push_back(tracker.asString());
			log_->debug() << "Added BitTorrent tracker: " << tracker.asString();
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
