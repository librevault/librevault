/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../bttracker/TrackerConnection.h"
#include "../../Session.h"

#include "../../Directory.h"

namespace librevault {
namespace p2p {

TrackerConnection::TrackerConnection(url tracker_address, Session& session) : Announcer(session), tracker_address(std::move(tracker_address)) {}

TrackerConnection::~TrackerConnection() {}

TrackerConnection::infohash TrackerConnection::get_infohash(const syncfs::Key& key) const {
	infohash ih; std::copy(key.get_Hash().begin(), key.get_Hash().begin()+ih.size(), ih.data());
	return ih;
}

TrackerConnection::seconds TrackerConnection::get_min_interval() const {
	return seconds(session.get_options().get<int>("discovery.bttracker.min_interval"));
}

} /* namespace p2p */
} /* namespace librevault */
