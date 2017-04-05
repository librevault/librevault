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
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <QTimer>

namespace librevault {

Q_LOGGING_CATEGORY(log_upnp, "portmapping.upnp")

UPnPService::UPnPService(PortMapper& parent) {
	upnp_urls = std::make_unique<UPNPUrls>();
	upnp_data = std::make_unique<IGDdatas>();

	DevListWrapper devlist;

	if(!UPNP_GetValidIGD(devlist.devlist, upnp_urls.get(), upnp_data.get(), lanaddr.data(), lanaddr.size())) {
		qCDebug(log_upnp) << "IGD not found. e:" << strerror(errno);
		throw std::runtime_error("IGD not found");
	}

	qCDebug(log_upnp) << "Found IGD:" << upnp_urls->controlURL;
}

UPnPService::~UPnPService() {
	mappings_.clear();

	FreeUPNPUrls(upnp_urls.get());
	upnp_urls.reset();
	upnp_data.reset();
}

void UPnPService::addPort(QString id, Mapping mapping) {
	mappings_[id] = std::make_shared<PortMapping>(*this, mapping);
	connect(mappings_[id].get(), &PortMapping::portMapped, [this, id](quint16 mapped){emit portMapped(id, mapped);});
}

void UPnPService::removePort(QString id) {
	mappings_.remove(id);
}

/* DevListWrapper */
DevListWrapper::DevListWrapper() {
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

DevListWrapper::~DevListWrapper() {
	freeUPNPDevlist(devlist);
}

/* PortMapping */
PortMapping::PortMapping(UPnPService& parent, Mapping mapping) : parent_(parent), mapping_(mapping) {
	int err = UPNP_AddPortMapping(parent_.upnp_urls->controlURL,
		parent_.upnp_data->first.servicetype,
		std::to_string(mapping.orig_port).c_str(),
		std::to_string(mapping.orig_port).c_str(),
		parent_.lanaddr.data(),
		mapping.description.toUtf8().data(),
		get_literal_protocol(mapping.protocol),
		nullptr,
		nullptr);
	if(!err)
		emit portMapped(mapping.mapped_port);
	else
		qCDebug(log_upnp) << "UPnP port forwarding failed: Error" << err;
}

PortMapping::~PortMapping() {
	auto err = UPNP_DeletePortMapping(
		parent_.upnp_urls->controlURL,
		parent_.upnp_data->first.servicetype,
		std::to_string(mapping_.orig_port).c_str(),
		get_literal_protocol(mapping_.protocol),
		nullptr
	);
	if(err)
		qCDebug(log_upnp) << get_literal_protocol(mapping_.protocol) << "port" << mapping_.orig_port << "de-forwarding failed: Error" << err;
}

} /* namespace librevault */
