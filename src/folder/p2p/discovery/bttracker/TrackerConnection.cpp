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
#include "src/Client.h"
#include "src/folder/FolderGroup.h"
#include "src/folder/p2p/P2PProvider.h"
#include "src/folder/fs/FSFolder.h"

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

TrackerConnection::info_hash TrackerConnection::get_info_hash() const {
	info_hash ih; std::copy(group_ptr_->hash().begin(), group_ptr_->hash().begin()+std::min(ih.size(), group_ptr_->hash().size()), ih.begin());
	return ih;
}

TrackerConnection::peer_id TrackerConnection::get_peer_id() const {
	TrackerConnection::peer_id pid;

	std::string az_id = client_.config().current.discovery_bttracker_azureus_id;
	if(az_id.size() != 8)
		az_id = client_.config().defaults.discovery_bttracker_azureus_id;

	auto pubkey_bytes_left = pid.size() - az_id.size();

	std::copy(az_id.begin(), az_id.end(), pid.begin());
	std::copy(client_.p2p_provider()->node_key().public_key().begin(),
		client_.p2p_provider()->node_key().public_key().begin() + pubkey_bytes_left, pid.begin() + az_id.size());

	return pid;
}

} /* namespace librevault */
