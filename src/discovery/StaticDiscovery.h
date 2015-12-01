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

namespace librevault {

class StaticDiscovery : public DiscoveryService {
public:
	StaticDiscovery(Client& client, Exchanger& exchanger);
	virtual ~StaticDiscovery();

	void register_group(std::shared_ptr<ExchangeGroup> group_ptr);
	void unregister_group(std::shared_ptr<ExchangeGroup> group_ptr);
protected:
	std::set<std::shared_ptr<ExchangeGroup>> groups_;
	//std::chrono::seconds repeat_interval_ = std::chrono::seconds(0);

	std::string log_tag() const {return "[StaticDiscovery] ";}
};

} /* namespace librevault */