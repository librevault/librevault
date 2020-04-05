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

#include "NATPMPService.h"
#include "UPnPService.h"
#include "util/log.h"

Q_LOGGING_CATEGORY(log_portmapping, "portmapping")

namespace librevault {

PortMappingService::PortMappingService(QObject* parent) : QObject(parent) {
  SCOPELOG(log_portmapping);
  natpmp_ = new NATPMPService(*this);
  upnp_ = new UPnPService(*this);

  auto port_callback = [this](QString id, uint16_t port) {
    qCDebug(log_portmapping)
        << "Port mapped: " << mappings_[id].descriptor.port << " -> " << port;
    mappings_[id].port = port;
  };

  connect(natpmp_, &NATPMPService::portMapped, port_callback);
  connect(upnp_, &UPnPService::portMapped, port_callback);

  natpmp_->start();
  upnp_->start();
}

PortMappingService::~PortMappingService() {
  SCOPELOG(log_portmapping);
  mappings_.clear();
}

void PortMappingService::map(QString id,
                             MappingDescriptor descriptor,
                             QString description) {
  Mapping m;
  m.descriptor = descriptor;
  m.description = description;
  m.port = descriptor.port;
  mappings_[id] = std::move(m);

  natpmp_->map(id, descriptor, description);
  upnp_->map(id, descriptor, description);
}

void PortMappingService::unmap(const QString& id) {
  upnp_->unmap(id);
  natpmp_->unmap(id);

  mappings_.erase(id);
}

uint16_t PortMappingService::mapped_port(const QString& id) {
  auto it = mappings_.find(id);
  if (it != mappings_.end())
    return it->second.port;
  else
    return 0;
}

void PortMappingService::add_existing_mappings(
    PortMappingSubService* subservice) {
  for (auto& mapping : mappings_)
    subservice->map(mapping.first, mapping.second.descriptor,
                    mapping.second.description);
}

} /* namespace librevault */
