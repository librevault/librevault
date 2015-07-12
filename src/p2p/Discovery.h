/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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

#include "Multicast.h"
#include "bttracker/TrackerConnection.h"
#include "../types.h"
#include <spdlog/spdlog.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>
#include <map>

namespace librevault {
namespace p2p {

class Discovery {
	std::shared_ptr<spdlog::logger> log;

	std::unique_ptr<Multicast> multicast4, multicast6;
	std::list<std::unique_ptr<TrackerConnection>> torrent_trackers;

	Session& session;
public:
	Discovery(Session& session);
	virtual ~Discovery(){};

	void start_lsd_announcer();
	void start_tracker_announcer();
	void stop();
};

} /* namespace discovery */
} /* namespace librevault */
