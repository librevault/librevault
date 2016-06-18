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
#include "UDPTrackerConnection.h"
#include "Client.h"
#include "folder/FolderGroup.h"
#include "folder/p2p/P2PProvider.h"
#include "folder/fs/FSFolder.h"

namespace librevault {

TrackerConnection::TrackerConnection(url tracker_address,
                                     std::shared_ptr<FolderGroup> group_ptr,
                                     BTTrackerDiscovery& tracker_discovery,
                                     Client& client) :
		Loggable(client),
		client_(client),
		tracker_discovery_(tracker_discovery),
		tracker_address_(tracker_address),
		group_ptr_(group_ptr) {
	assert(tracker_address_.scheme == "udp");
	if(tracker_address_.port == 0)
		tracker_address_.port = 80;
}

TrackerConnection::~TrackerConnection() {}

btcompat::info_hash TrackerConnection::get_info_hash() const {
	return btcompat::get_info_hash(group_ptr_->hash());
}

btcompat::peer_id TrackerConnection::get_peer_id() const {
	return btcompat::get_peer_id(client_.p2p_provider()->node_key().public_key());
}

} /* namespace librevault */
