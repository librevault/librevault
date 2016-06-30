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
#include "pch.h"
#include "PortManager.h"

namespace librevault {

class PortMappingService {
public:
	using MappingDescriptor = PortManager::MappingDescriptor;
	boost::signals2::signal<void(std::string, uint16_t)> port_signal;

	virtual void add_port_mapping(const std::string& id, MappingDescriptor descriptor, std::string description) = 0;
	virtual void remove_port_mapping(const std::string& id) = 0;
};

} /* namespace librevault */
