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

NATPMPService::NATPMPService(Client& client, PortManager& parent) : PortMappingService(parent), Loggable(client, "NATPMPService"), client_(client) {
	Config::get()->config_changed.connect(std::bind(&NATPMPService::reload_config, this));
}
NATPMPService::~NATPMPService() {stop();}

bool NATPMPService::is_config_enabled() {
	return Config::get()->globals()["natpmp_enabled"].asBool();
}

void NATPMPService::start() {
	if(!is_config_enabled()) return;

	active = true;

	int natpmp_ec = initnatpmp(&natpmp, 0, 0);
	log_->trace() << log_tag() << "initnatpmp() = " << natpmp_ec;

	/* Adding all present mappings */
	{
		std::unique_lock<std::mutex> lk(get_mappings_mutex());
		for(auto& mapping : get_mappings()) {
			add_port_mapping(mapping.first, mapping.second.descriptor, mapping.second.description);
		}
		added_mapping_signal_conn = parent_.added_mapping_signal.connect([this](const std::string& id, MappingDescriptor descriptor, std::string description){
			add_port_mapping(id, descriptor, description);
		});
		removed_mapping_signal_conn = parent_.removed_mapping_signal.connect([this](const std::string& id){
			remove_port_mapping(id);
		});
	}
}

void NATPMPService::stop() {
	active = false;

	added_mapping_signal_conn.disconnect();
	removed_mapping_signal_conn.disconnect();

	mappings_.clear();
	closenatpmp(&natpmp);
}

void NATPMPService::reload_config() {
	bool config_enabled = is_config_enabled();

	if(config_enabled && !active)
		start();
	else if(!config_enabled && active)
		stop();
}

void NATPMPService::add_port_mapping(const std::string& id, MappingDescriptor descriptor, std::string description) {
	mappings_.erase(id);
	mappings_[id] = std::make_unique<PortMapping>(*this, id, descriptor);
}

void NATPMPService::remove_port_mapping(const std::string& id) {
	mappings_.erase(id);
}

/* NATPMPService::PortMapping */
NATPMPService::PortMapping::PortMapping(NATPMPService& parent, std::string id, MappingDescriptor descriptor) :
	parent_(parent),
	id_(id),
	descriptor_(descriptor),
	maintain_mapping_(parent.client_.network_ios(), [this](PeriodicProcess& process){send_request(process);}) {
	maintain_mapping_.invoke_post();
}

NATPMPService::PortMapping::~PortMapping() {
	active = false;
	maintain_mapping_.wait();
	maintain_mapping_.invoke();
}

void NATPMPService::PortMapping::send_request(PeriodicProcess& process) {
	int natpmp_ec = sendnewportmappingrequest(
		&parent_.natpmp,
		descriptor_.protocol == PortManager::MappingDescriptor::TCP ? NATPMP_PROTOCOL_TCP : NATPMP_PROTOCOL_UDP,
		descriptor_.port,
		descriptor_.port,
		active ? Config::get()->globals()["natpmp_lifetime"].asUInt() : 0
	);
	parent_.log_->trace() << parent_.log_tag() << "sendnewportmappingrequest() = " << natpmp_ec;

	natpmpresp_t natpmp_resp;
	do {
		natpmp_ec = readnatpmpresponseorretry(&parent_.natpmp, &natpmp_resp);
	}while(natpmp_ec == NATPMP_TRYAGAIN);
	parent_.log_->trace() << parent_.log_tag() << "readnatpmpresponseorretry() = " << natpmp_ec;

	std::chrono::seconds next_request;
	if(natpmp_ec >= 0) {
		parent_.port_signal(id_, natpmp_resp.pnu.newportmapping.mappedpublicport);
		next_request = seconds(natpmp_resp.pnu.newportmapping.lifetime);
	}else{
		parent_.log_->debug() << parent_.log_tag() << "Could not set up port mapping";
		next_request = seconds(Config::get()->globals()["natpmp_lifetime"].asUInt());
	}

	if(active)
		process.invoke_after(next_request);
}

} /* namespace librevault */
