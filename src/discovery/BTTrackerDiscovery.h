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
#include "../pch.h"
#include "DiscoveryService.h"
#include "src/util/parse_url.h"

namespace librevault {

class TrackerConnection;

class BTTrackerDiscovery : public DiscoveryService {
public:
	BTTrackerDiscovery(Client& client, Exchanger& exchanger);
	virtual ~BTTrackerDiscovery();

	void register_group(std::shared_ptr<ExchangeGroup> group_ptr);
	void unregister_group(std::shared_ptr<ExchangeGroup> group_ptr);
protected:
	std::unordered_multimap<std::shared_ptr<ExchangeGroup>, std::unique_ptr<TrackerConnection>> groups_;

	std::list<url> trackers_;
	std::string log_tag() const {return "[BTTrackerDiscovery] ";}
};

} /* namespace librevault */