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
#include <utility>

#include "control/Config.h"
#include "util/log.h"

Q_LOGGING_CATEGORY(log_upnp, "upnp")

namespace librevault {

const char* get_literal_protocol(int protocol) { return protocol == QAbstractSocket::TcpSocket ? "TCP" : "UDP"; }

UPnPService::UPnPService(PortMappingService& parent) : PortMappingSubService(parent) {}
UPnPService::~UPnPService() { stop(); }

bool UPnPService::is_config_enabled() { return Config::get()->getGlobal("upnp_enabled").toBool(); }

void UPnPService::start() {
  upnp_urls = std::make_unique<UPNPUrls>();
  upnp_data = std::make_unique<IGDdatas>();

  if (!is_config_enabled()) return;

  /* Discovering IGD */
  int error = UPNPDISCOVER_SUCCESS;
  std::unique_ptr<UPNPDev, decltype(&freeUPNPDevlist)> devlist(upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, &error),
                                                               &freeUPNPDevlist);
  if (error != UPNPDISCOVER_SUCCESS) throw std::runtime_error(strerror(errno));

  if (!UPNP_GetValidIGD(devlist.get(), upnp_urls.get(), upnp_data.get(), lanaddr.data(), lanaddr.size())) {
    qCDebug(log_upnp) << "IGD not found. e: " << strerror(errno);
    return;
  }

  qCDebug(log_upnp) << "Found IGD: " << upnp_urls->controlURL;

  add_existing();
}

void UPnPService::stop() {
  mappings_.clear();

  FreeUPNPUrls(upnp_urls.get());
  upnp_urls.reset();
  upnp_data.reset();
}

void UPnPService::map(const QString& id, MappingDescriptor descriptor, const QString& description) {
  mappings_.erase(id);
  mappings_[id] = std::make_shared<UPnPMapping>(*this, id, descriptor, description);
}

void UPnPService::unmap(const QString& id) { mappings_.erase(id); }

UPnPMapping::UPnPMapping(UPnPService& parent, QString id, MappingDescriptor descriptor, const QString& description)
    : parent_(parent), id_(std::move(id)), descriptor_(descriptor) {
  int err = UPNP_AddPortMapping(parent_.upnp_urls->controlURL, parent_.upnp_data->first.servicetype,
                                std::to_string(descriptor.port).c_str(), std::to_string(descriptor.port).c_str(),
                                parent_.lanaddr.data(), description.toStdString().data(),
                                get_literal_protocol(descriptor.protocol), nullptr, nullptr);
  if (!err)
    parent_.portMapped(id_, descriptor.port);
  else
    qCDebug(log_upnp) << "UPnP port forwarding failed: Error " << err;
}

UPnPMapping::~UPnPMapping() {
  auto err = UPNP_DeletePortMapping(parent_.upnp_urls->controlURL, parent_.upnp_data->first.servicetype,
                                    std::to_string(descriptor_.port).c_str(),
                                    get_literal_protocol(descriptor_.protocol), nullptr);
  if (err)
    qCDebug(log_upnp) << get_literal_protocol(descriptor_.protocol) << " port " << descriptor_.port
                      << " de-forwarding failed: Error " << err;
}

}  // namespace librevault
