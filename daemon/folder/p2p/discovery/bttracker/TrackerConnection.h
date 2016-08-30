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
#pragma once
#include "pch.h"
#include "util/Loggable.h"
#include "util/parse_url.h"
#include "../btcompat.h"

namespace librevault {

class FolderGroup;
class P2PProvider;
class Client;
class BTTrackerDiscovery;

// BEP-0015 partial implementation (without scrape mechanism)
class TrackerConnection : protected Loggable {
public:
	TrackerConnection(url tracker_address, std::shared_ptr<FolderGroup> group_ptr, BTTrackerDiscovery& tracker_discovery, Client& client);
	virtual ~TrackerConnection();
protected:
	enum class Event : int32_t {
		EVENT_NONE=0, EVENT_COMPLETED=1, EVENT_STARTED=2, EVENT_STOPPED=3
	};

	Client& client_;
	BTTrackerDiscovery& tracker_discovery_;

	url tracker_address_;
	std::shared_ptr<FolderGroup> group_ptr_;

	unsigned int announced_times_ = 0;

	std::string log_tag() const {return std::string("[bttracker: ") + std::string(tracker_address_) + "] ";}

	btcompat::info_hash get_info_hash() const;
	btcompat::peer_id get_peer_id() const;
};

} /* namespace librevault */
