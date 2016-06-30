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
#include "NATPMPService.h"
#include "Client.h"
#include "control/Config.h"

namespace librevault {

NATPMPService::NATPMPService(Client& client) :
	Loggable(client, "NATPMPService"), client_(client) {

	int natpmp_ec = initnatpmp(&natpmp, 0, 0);
	log_->trace() << log_tag() << "initnatpmp() = " << natpmp_ec;

	Config::get()->config_changed.connect(std::bind(&NATPMPService::reload_config, this));
}

NATPMPService::~NATPMPService() {
	mappings_.clear();
	closenatpmp(&natpmp);
}

void NATPMPService::reload_config() {
	for(auto& mapping : mappings_)
		mapping.second->reload_config();
}

void NATPMPService::add_port_mapping(const std::string& id, MappingDescriptor descriptor, std::string description) {
	mappings_.erase(id);
	mappings_[id] = std::make_unique<PortMapping>(*this, descriptor);
}

void NATPMPService::remove_port_mapping(const std::string& id) {
	mappings_.erase(id);
}

/* NATPMPService::PortMapping */
NATPMPService::PortMapping::PortMapping(NATPMPService& parent, MappingDescriptor descriptor) : parent_(parent), descriptor_(descriptor), maintain_timer_(parent.client_.network_ios()) {
	reload_config();
}

NATPMPService::PortMapping::~PortMapping() {
	if(active) {
		maintain_timer_.cancel();
		send_request(true);
	}
}

void NATPMPService::PortMapping::reload_config() {
	bool config_enabled = Config::get()->globals()["natpmp_enabled"].asBool();
	if(config_enabled && !active) {
		active = true;
		send_request(false);
	}else if(!config_enabled && active) {
		active = false;
		send_request(true);
	}
}

void NATPMPService::PortMapping::send_request(bool unmap, const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) return;
	std::unique_lock<std::mutex> natpmp_lk(parent_.natpmp_lock_);

	int natpmp_ec = sendnewportmappingrequest(
		&parent_.natpmp,
		descriptor_.protocol == PortManager::MappingDescriptor::TCP ? NATPMP_PROTOCOL_TCP : NATPMP_PROTOCOL_UDP,
		descriptor_.port,
		unmap ? 0 : descriptor_.port,
		Config::get()->globals()["natpmp_lifetime"].asUInt()
	);
	parent_.log_->trace() << parent_.log_tag() << "sendnewportmappingrequest() = " << natpmp_ec;

	natpmpresp_t natpmp_resp;
	do {
		natpmp_ec = readnatpmpresponseorretry(&parent_.natpmp, &natpmp_resp);
	}while(natpmp_ec == NATPMP_TRYAGAIN);
	parent_.log_->trace() << parent_.log_tag() << "readnatpmpresponseorretry() = " << natpmp_ec;

	uint16_t public_port;
	std::chrono::seconds next_request;
	if(natpmp_ec >= 0) {
		public_port = natpmp_resp.pnu.newportmapping.mappedpublicport;
		parent_.log_->debug() << parent_.log_tag() << "Successfully set up port mapping " << descriptor_.port << " -> "
			<< public_port;
		next_request = seconds(natpmp_resp.pnu.newportmapping.lifetime);
	}else{
		public_port = 0;
		parent_.log_->debug() << parent_.log_tag() << "Could not set up port mapping";
		next_request = seconds(Config::get()->globals()["natpmp_lifetime"].asUInt());
	}

	if(!unmap) {
		maintain_timer_.expires_from_now(next_request);
		maintain_timer_.async_wait(std::bind(&NATPMPService::PortMapping::send_request, this, true, std::placeholders::_1));
	}
}

} /* namespace librevault */
