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
#include "PortMappingService.h"

#include <control/Config.h>

#include "NATPMPService.h"
#include "UPnPService.h"
#include "util/log.h"

Q_LOGGING_CATEGORY(log_portmapping, "portmapping")

namespace librevault {

PortMappingService::PortMappingService(QObject* parent) : QObject(parent) {
  SCOPELOG(log_portmapping);
  natpmp_ = new NATPMPService(*this);
  upnp_ = new UPnPService(*this);

  connect(natpmp_, &NATPMPService::portMapped, this, &PortMappingService::portCallback);
  connect(upnp_, &UPnPService::portMapped, this, &PortMappingService::portCallback);

  if (Config::get()->getGlobal("natpmp_enabled").toBool()) natpmp_->start();
  if (Config::get()->getGlobal("upnp_enabled").toBool()) upnp_->start();
}

PortMappingService::~PortMappingService() {
  SCOPELOG(log_portmapping);
  mappings_.clear();
}

void PortMappingService::map(const MappingRequest& request) {
  Mapping m;
  m.request = request;
  mappings_[request.id] = std::move(m);

  natpmp_->map(request);
  upnp_->map(request);
}

void PortMappingService::unmap(const QString& id) {
  upnp_->unmap(id);
  natpmp_->unmap(id);

  mappings_.remove(id);
}

uint16_t PortMappingService::mappedPort(const QString& id) {
  if (mappings_.contains(id)) return mappings_[id].result.external_port;
  return 0;
}

void PortMappingService::add_existing_mappings(PortMappingSubService* subservice) {
  for (auto& mapping : mappings_.values()) subservice->map(mapping.request);
}

void PortMappingService::portCallback(const MappingResult& result) {
  mappings_[result.id].result = result;
  qCDebug(log_portmapping) << "Port" << result.id << "mapped:" << mappings_[result.id].request.port << "->"
                           << mappings_[result.id].result.external_port;
}

}  // namespace librevault
