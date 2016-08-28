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
#include "PortManager.h"

namespace librevault {

class PortMappingService {
public:
	PortMappingService(PortManager& parent) : parent_(parent) {}

	using MappingDescriptor = PortManager::MappingDescriptor;
	boost::signals2::signal<void(std::string, uint16_t)> port_signal;

protected:
	PortManager& parent_;
	const decltype(PortManager::mappings_)& get_mappings() {
		return parent_.mappings_;
	}
	decltype(PortManager::mappings_mutex_)& get_mappings_mutex() {
		return parent_.mappings_mutex_;
	}

	boost::signals2::scoped_connection added_mapping_signal_conn;
	boost::signals2::scoped_connection removed_mapping_signal_conn;
};

} /* namespace librevault */
