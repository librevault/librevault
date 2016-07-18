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

UPnPService::UPnPService(Client& client, PortManager& parent) : PortMappingService(parent), Loggable(client, "UPnPService"), client_(client) {
	Config::get()->config_changed.connect(std::bind(&UPnPService::reload_config, this));
}
UPnPService::~UPnPService() {stop();}

bool UPnPService::is_config_enabled() {
	return Config::get()->globals()["upnp_enabled"].asBool();
}

void UPnPService::start() {
	if(!is_config_enabled()) return;

	active = true;

	upnp_urls = std::make_unique<UPNPUrls>();
	upnp_data = std::make_unique<IGDdatas>();

	/* Discovering IGD */
	DevListWrapper devlist;

	if(!UPNP_GetValidIGD(devlist.devlist, upnp_urls.get(), upnp_data.get(), lanaddr.data(), lanaddr.size())) {
		log_->debug() << log_tag() << "IGD not found. e: " << strerror(errno);
		return;
	}

	log_->debug() << log_tag() << "Found IGD: " << upnp_urls->controlURL;

	/* Adding all present mappings */
	{
		std::shared_lock<std::shared_timed_mutex> lk(get_mappings_mutex());
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

void UPnPService::stop() {
	active = false;

	added_mapping_signal_conn.disconnect();
	removed_mapping_signal_conn.disconnect();

	mappings_.clear();
	FreeUPNPUrls(upnp_urls.get());
}

void UPnPService::reload_config() {
	bool config_enabled = is_config_enabled();

	if(config_enabled && !active)
		start();
	else if(!config_enabled && active)
		stop();
}

void UPnPService::add_port_mapping(const std::string& id, MappingDescriptor descriptor, std::string description) {
	mappings_.erase(id);
	mappings_[id] = std::make_shared<PortMapping>(*this, id, descriptor, description);
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
UPnPService::PortMapping::PortMapping(UPnPService& parent, std::string id, MappingDescriptor descriptor, const std::string description) : parent_(parent), id_(id), descriptor_(descriptor) {
	int err = UPNP_AddPortMapping(parent_.upnp_urls->controlURL,
		parent_.upnp_data->first.servicetype,
		std::to_string(descriptor.port).c_str(),
		std::to_string(descriptor.port).c_str(),
		parent_.lanaddr.data(),
		description.data(),
		get_literal_protocol(descriptor.protocol),
		nullptr,
		nullptr);
	if(!err)
		parent_.port_signal(id, descriptor.port);
	else
		parent_.log_->debug() << parent_.log_tag() << "UPnP port forwarding failed: Error " << err;
}

UPnPService::PortMapping::~PortMapping() {
	auto err = UPNP_DeletePortMapping(
		parent_.upnp_urls->controlURL,
		parent_.upnp_data->first.servicetype,
		std::to_string(descriptor_.port).c_str(),
		get_literal_protocol(descriptor_.protocol),
		nullptr
	);
	if(err)
		parent_.log_->debug() << parent_.log_tag() << get_literal_protocol(descriptor_.protocol) << " port " << descriptor_.port << " de-forwarding failed: Error " << err;
}

} /* namespace librevault */
