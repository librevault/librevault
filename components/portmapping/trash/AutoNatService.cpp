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
#include "AutoNatService.h"
#include "NatPmpService.h"
#include "UpnpService.h"

namespace librevault {

AutoNatService::AutoNatService(QObject* parent) : GenericNatService(parent) {
  nested_services_.push_back(new NatPmpService(this));
  nested_services_.push_back(new UpnpService(this));
}

bool AutoNatService::isReady() {
  for (auto& service : nested_services_)
    if (service->isReady()) return true;
  return false;
}

PortMapping* AutoNatService::createMapping(const MappingRequest& request) {
  QList<PortMapping*> mappings;
  for (auto& service : nested_services_) mappings.push_back(service->createMapping(request));

  return new AutoPortMapping(mappings, request, this);
}

AutoPortMapping::AutoPortMapping(
    const QList<PortMapping*>& mappings, const MappingRequest& request, GenericNatService* parent)
    : PortMapping(request, parent), mappings_(mappings) {
  for (auto& mapping : mappings_) {
    connect(mapping, &PortMapping::mapped, this, &PortMapping::mapped);
    mapping->setParent(this);
  }
}

void AutoPortMapping::map() {
  if(current_ && current_->isServiceReady())
    return current_->map();

  for (auto& mapping : mappings_) {
    if (mapping->isServiceReady()) {
      current_ = mapping;
      return current_->map();
    }
  }

  current_ = nullptr;
}

void AutoPortMapping::unmap() {
  if(current_)
    return current_->unmap();
}

}  // namespace librevault
