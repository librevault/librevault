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
#include "UPnPService.h"
#include <control/Config.h>
#include <Client.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>

namespace librevault {

UPnPService::UPnPService(Client& client) :
	Loggable(client, "UPnPService"), client_(client) {

	reload_config();
	Config::get()->config_changed.connect([this]{UPnPService::reload_config();});
	discover_igd();
}

UPnPService::~UPnPService() {
	mappings_.clear();
	FreeUPNPUrls(&upnp_urls);
}

void UPnPService::discover_igd() {
	DevListWrapper devlist;

	if(!UPNP_GetValidIGD(devlist.devlist, &upnp_urls, &upnp_data, lanaddr.data(), lanaddr.size()))
		throw std::runtime_error(strerror(errno));

	log_->debug() << log_tag() << "Found IGD: " << upnp_urls.controlURL;
}

void UPnPService::reload_config() {
	bool new_enabled = Config::get()->globals()["natpmp_enabled"].asBool();
	seconds new_lifetime = seconds(Config::get()->globals()["natpmp_lifetime"].asUInt64());

	enabled_ = new_enabled;
}

void UPnPService::add_port_mapping(const std::string& id, MappingDescriptor descriptor, std::string description) {
	mappings_.erase(id);
	mappings_[id] = std::make_shared<PortMapping>(*this, descriptor, description);
}

void UPnPService::remove_port_mapping(const std::string& id) {
	mappings_.erase(id);
}

/* UPnPService::DevListWrapper */
UPnPService::DevListWrapper::DevListWrapper() {
	int error = UPNPDISCOVER_SUCCESS;
	devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, &error);
	if(error != UPNPDISCOVER_SUCCESS) {
		throw std::runtime_error(strerror(errno));
	}
}

UPnPService::DevListWrapper::~DevListWrapper() {
	freeUPNPDevlist(devlist);
}

/* UPnPService::PortMapping */
UPnPService::PortMapping::PortMapping(UPnPService& parent, MappingDescriptor descriptor, const std::string description) : parent_(parent), descriptor_(descriptor) {
	int err = UPNP_AddPortMapping(parent_.upnp_urls.controlURL,
		parent_.upnp_data.first.servicetype,
		std::to_string(descriptor.port).c_str(),
		std::to_string(descriptor.port).c_str(),
		parent_.lanaddr.data(),
		description.data(),
		get_literal_protocol(descriptor.protocol),
		nullptr,
		nullptr);
	if(err)
		parent_.log_->debug() << parent_.log_tag() << "UPnP port forwarding failed: Error " << err;
}

UPnPService::PortMapping::~PortMapping() {
	auto err = UPNP_DeletePortMapping(
		parent_.upnp_urls.controlURL,
		parent_.upnp_data.first.servicetype,
		std::to_string(descriptor_.port).c_str(),
		get_literal_protocol(descriptor_.protocol),
		nullptr
	);
	if(err)
		parent_.log_->debug() << parent_.log_tag() << get_literal_protocol(descriptor_.protocol) << " port " << descriptor_.port << " de-forwarding failed: Error " << err;
}

} /* namespace librevault */
