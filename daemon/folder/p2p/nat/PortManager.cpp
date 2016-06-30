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
#include "PortManager.h"
#include "Client.h"
#include "NATPMPService.h"
#include "UPnPService.h"

namespace librevault {

PortManager::PortManager(Client& client, P2PProvider& provider) :
	Loggable(client, "PortManager"), client_(client), provider_(provider) {
	natpmp_service_ = std::make_unique<NATPMPService>(client_);
	upnp_service_ = std::make_unique<UPnPService>(client_);

	natpmp_service_->port_signal.connect([this](std::string id, uint16_t port){
		mappings_[id].second = port;
	});
	upnp_service_->port_signal.connect([this](std::string id, uint16_t port){
		mappings_[id].second = port;
	});
}

PortManager::~PortManager() {
	upnp_service_.reset();
	natpmp_service_.reset();
}

void PortManager::add_port_mapping(const std::string& id, MappingDescriptor descriptor, std::string description) {
	mappings_[id] = {descriptor, 0};
	upnp_service_->add_port_mapping(id, descriptor, description);
	natpmp_service_->add_port_mapping(id, descriptor, description);
}

void PortManager::remove_port_mapping(const std::string& id) {
	natpmp_service_->remove_port_mapping(id);
	upnp_service_->remove_port_mapping(id);
}

uint16_t PortManager::get_port_mapping(const std::string& id) {
	auto it = mappings_.find(id);
	if(it != mappings_.end())
		return it->second.second;
	else
		return 0;
}

} /* namespace librevault */
