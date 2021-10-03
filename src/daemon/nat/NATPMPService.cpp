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
 */
#include "NATPMPService.h"

#include <control/Config.h>
#include <util/log.h>

Q_LOGGING_CATEGORY(log_natpmp, "natpmp")

namespace librevault {

NATPMPService::NATPMPService(PortMappingService &parent) : PortMappingSubService(parent) {}
NATPMPService::~NATPMPService() { stop(); }

void NATPMPService::start() {
  int natpmp_ec = initnatpmp(&natpmp, 0, 0);
  qCDebug(log_natpmp) << "initnatpmp() = " << natpmp_ec;

  if (natpmp_ec == 0)
    for (const auto &request : requests_.values())
      mappings_[request.id] = std::make_unique<NATPMPMapping>(*this, request);
}

void NATPMPService::stop() {
  mappings_.clear();
  closenatpmp(&natpmp);
}

void NATPMPService::map(const MappingRequest &request) {
  requests_[request.id] = request;
  if (active_) mappings_[request.id] = std::make_unique<NATPMPMapping>(*this, request);
}

void NATPMPService::unmap(const QString &id) {
  mappings_.erase(id);
  requests_.remove(id);
}

/* NATPMPMapping */
NATPMPMapping::NATPMPMapping(NATPMPService &parent, MappingRequest request) : parent_(parent), request_(request) {
  timer_ = new QTimer(this);
  timer_->setTimerType(Qt::VeryCoarseTimer);

  QTimer::singleShot(0, this, &NATPMPMapping::sendPeriodicRequest);
}

NATPMPMapping::~NATPMPMapping() { sendZeroRequest(); }

void NATPMPMapping::sendPeriodicRequest() {
  int natpmp_ec = sendnewportmappingrequest(
      &parent_.natpmp, request_.protocol == QAbstractSocket::TcpSocket ? NATPMP_PROTOCOL_TCP : NATPMP_PROTOCOL_UDP,
      request_.port, request_.port, Config::get()->getGlobal("natpmp_lifetime").toUInt());
  qCDebug(log_natpmp) << "sendnewportmappingrequest() = " << natpmp_ec;

  natpmpresp_t natpmp_resp;
  do {
    natpmp_ec = readnatpmpresponseorretry(&parent_.natpmp, &natpmp_resp);
  } while (natpmp_ec == NATPMP_TRYAGAIN);
  qCDebug(log_natpmp) << "readnatpmpresponseorretry() = " << natpmp_ec;

  int next_request_sec;
  if (natpmp_ec >= 0) {
    parent_.portMapped({request_.id, natpmp_resp.pnu.newportmapping.mappedpublicport});
    next_request_sec = natpmp_resp.pnu.newportmapping.lifetime;
  } else {
    qCDebug(log_natpmp) << "Could not set up port mapping";
    next_request_sec = Config::get()->getGlobal("natpmp_lifetime").toUInt();
  }

  timer_->start(next_request_sec * 1000);
}

void NATPMPMapping::sendZeroRequest() {
  int natpmp_ec = sendnewportmappingrequest(
      &parent_.natpmp, request_.protocol == QAbstractSocket::TcpSocket ? NATPMP_PROTOCOL_TCP : NATPMP_PROTOCOL_UDP,
      request_.port, request_.port, 0);
  qCDebug(log_natpmp) << "sendnewportmappingrequest() = " << natpmp_ec;
}

}  // namespace librevault
