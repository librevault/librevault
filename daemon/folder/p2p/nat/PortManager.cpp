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
	natpmp_service_ = std::make_unique<NATPMPService>(client_, *this);
	upnp_service_ = std::make_unique<UPnPService>(client_, *this);

	auto port_callback = [this](std::string id, uint16_t port) {
		log_->debug() << log_tag() << "Port mapped: " << mappings_[id].descriptor.port << " -> " << port;
		mappings_[id].port = port;
	};
	natpmp_service_->port_signal.connect(port_callback);
	upnp_service_->port_signal.connect(port_callback);

	client.ios().post([this]{natpmp_service_->start();});
	client.ios().post([this]{upnp_service_->start();});
}

PortManager::~PortManager() {
	upnp_service_.reset();
	natpmp_service_.reset();
}

void PortManager::add_port_mapping(const std::string& id, MappingDescriptor descriptor, std::string description) {
	std::unique_lock<std::shared_timed_mutex> lk(mappings_mutex_);
	Mapping m;
	m.descriptor = descriptor;
	m.description = description;
	m.port = descriptor.port;
	mappings_[id] = std::move(m);
	added_mapping_signal(id, descriptor, description);
}

void PortManager::remove_port_mapping(const std::string& id) {
	std::unique_lock<std::shared_timed_mutex> lk(mappings_mutex_);
	mappings_.erase(id);
	removed_mapping_signal(id);
}

uint16_t PortManager::get_port_mapping(const std::string& id) {
	auto it = mappings_.find(id);
	if(it != mappings_.end())
		return it->second.port;
	else
		return 0;
}

} /* namespace librevault */
