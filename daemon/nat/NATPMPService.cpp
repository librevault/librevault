/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "NATPMPService.h"
#include "control/Config.h"
#include <util/log.h>

namespace librevault {

NATPMPService::NATPMPService(PortMappingService& parent, io_service& ios) : PortMappingSubService(parent), ios_(ios) {
	Config::get()->config_changed.connect(std::bind(&NATPMPService::reload_config, this));
}
NATPMPService::~NATPMPService() {stop();}

bool NATPMPService::is_config_enabled() {
	return Config::get()->global_get("natpmp_enabled").asBool();
}

void NATPMPService::start() {
	if(!is_config_enabled()) return;

	ios_.post([this]{
		int natpmp_ec = initnatpmp(&natpmp, 0, 0);
		LOGT("initnatpmp() = " << natpmp_ec);

		if(natpmp_ec == 0) {
			active = true;
			add_existing_mappings();
		}
	});
}

void NATPMPService::stop() {
	active = false;

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
	maintain_mapping_(parent.ios_, [this](PeriodicProcess& process){send_request(process);}) {
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
		descriptor_.protocol == SOCK_STREAM ? NATPMP_PROTOCOL_TCP : NATPMP_PROTOCOL_UDP,
		descriptor_.port,
		descriptor_.port,
		active ? Config::get()->global_get("natpmp_lifetime").asUInt() : 0
	);
	LOGT("sendnewportmappingrequest() = " << natpmp_ec);

	natpmpresp_t natpmp_resp;
	do {
		natpmp_ec = readnatpmpresponseorretry(&parent_.natpmp, &natpmp_resp);
	}while(natpmp_ec == NATPMP_TRYAGAIN);
	LOGT("readnatpmpresponseorretry() = " << natpmp_ec);

	std::chrono::seconds next_request;
	if(natpmp_ec >= 0) {
		parent_.port_signal(id_, natpmp_resp.pnu.newportmapping.mappedpublicport);
		next_request = std::chrono::seconds(natpmp_resp.pnu.newportmapping.lifetime);
	}else{
		LOGD("Could not set up port mapping");
		next_request = std::chrono::seconds(Config::get()->global_get("natpmp_lifetime").asUInt());
	}

	if(active)
		process.invoke_after(next_request);
}

} /* namespace librevault */
