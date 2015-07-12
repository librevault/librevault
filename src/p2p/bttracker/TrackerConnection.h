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
#pragma once

#include "Announcer.h"
#include "../../syncfs/Key.h"
#include "../../net/parse_url.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <queue>

namespace librevault {
namespace p2p {

using boost::asio::io_service;
using boost::property_tree::ptree;


class TrackerConnection : public Announcer {
protected:
	url tracker_address;

	using infohash = std::array<uint8_t, 20>;
	std::map<infohash, std::shared_ptr<Directory>> infohashes;

	virtual void announce(const infohash& ih){}
	infohash get_infohash(const syncfs::Key& key) const;
public:
	TrackerConnection(url tracker_address, Session& session);
	virtual ~TrackerConnection();

	seconds get_min_interval() const override;
};

} /* namespace p2p */
} /* namespace librevault */
