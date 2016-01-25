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
#pragma once
#include "src/pch.h"
#include "src/util/Loggable.h"
#include "src/util/parse_url.h"

namespace librevault {

class ExchangeGroup;
class P2PProvider;
class Client;
class BTTrackerDiscovery;

// BEP-0015 partial implementation (without scrape mechanism)
class TrackerConnection : protected Loggable {
public:
	TrackerConnection(url tracker_address, std::shared_ptr<ExchangeGroup> group_ptr, BTTrackerDiscovery& tracker_discovery, Client& client);
	virtual ~TrackerConnection();
protected:
	enum class Event : int32_t {
		EVENT_NONE=0, EVENT_COMPLETED=1, EVENT_STARTED=2, EVENT_STOPPED=3
	};

	using info_hash = std::array<uint8_t, 20>;
	using peer_id = std::array<uint8_t, 20>;

	Client& client_;
	BTTrackerDiscovery& tracker_discovery_;

	url tracker_address_;
	std::shared_ptr<ExchangeGroup> group_ptr_;

	unsigned int announced_times_ = 0;

	std::string log_tag() const {return std::string("[bttracker: ") + std::string(tracker_address_) + "] ";}

	info_hash get_info_hash() const;
	peer_id get_peer_id() const;
};

} /* namespace librevault */
