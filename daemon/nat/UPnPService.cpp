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
#include "UPnPService.h"
#include "control/Config.h"
#include "util/log.h"
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <QTimer>

namespace librevault {

UPnPService::UPnPService(PortMappingService& parent) : PortMappingSubService(parent) {
	//Config::get()->config_changed.connect(std::bind(&UPnPService::reload_config, this));
}
UPnPService::~UPnPService() {stop();}

bool UPnPService::is_config_enabled() {
	return Config::get()->global_get("upnp_enabled").toBool();
}

void UPnPService::start() {
	upnp_urls = std::make_unique<UPNPUrls>();
	upnp_data = std::make_unique<IGDdatas>();

	if(!is_config_enabled()) return;

	active = true;

	/* Discovering IGD */
	DevListWrapper devlist;

	if(!UPNP_GetValidIGD(devlist.devlist, upnp_urls.get(), upnp_data.get(), lanaddr.data(), lanaddr.size())) {
		LOGD("IGD not found. e: " << strerror(errno));
		return;
	}

	LOGD("Found IGD: " << upnp_urls->controlURL);

	add_existing_mappings();
}

void UPnPService::stop() {
	active = false;

	mappings_.clear();

	FreeUPNPUrls(upnp_urls.get());
	upnp_urls.reset();
	upnp_data.reset();
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
#if MINIUPNPC_API_VERSION >= 14
	devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, &error);
#else
	devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, &error);
#endif
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
		LOGD("UPnP port forwarding failed: Error " << err);
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
		LOGD(get_literal_protocol(descriptor_.protocol) << " port " << descriptor_.port << " de-forwarding failed: Error " << err);
}

} /* namespace librevault */
