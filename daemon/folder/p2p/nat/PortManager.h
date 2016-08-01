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
#include <util/Loggable.h>

namespace librevault {

class Client;
class P2PProvider;

class NATPMPService;
class UPnPService;

class PortManager : public Loggable {
	friend class PortMappingService;
public:
	struct MappingDescriptor {
		enum Protocol {
			TCP = SOCK_STREAM,
			UDP = SOCK_DGRAM
		} protocol;
		uint16_t port;

		MappingDescriptor() {}
		MappingDescriptor(Protocol new_protocol, uint16_t new_port) : protocol(new_protocol), port(new_port) {}
	};

	PortManager(Client& client, P2PProvider& provider);
	virtual ~PortManager();

	void add_port_mapping(const std::string& id, MappingDescriptor descriptor, std::string description);
	void remove_port_mapping(const std::string& id);
	uint16_t get_port_mapping(const std::string& id);

	/**/
	boost::signals2::signal<void(const std::string&, MappingDescriptor, std::string)> added_mapping_signal;
	boost::signals2::signal<void(const std::string&)> removed_mapping_signal;

private:
	Client& client_;
	P2PProvider& provider_;

	std::mutex mappings_mutex_;

	struct Mapping {
		MappingDescriptor descriptor;
		std::string description;
		uint16_t port;
	};
	std::map<std::string, Mapping> mappings_;

	std::unique_ptr<NATPMPService> natpmp_service_;
	std::unique_ptr<UPnPService> upnp_service_;
};

} /* namespace librevault */
