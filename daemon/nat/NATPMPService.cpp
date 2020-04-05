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

#include <util/log.h>

#include "control/Config.h"

Q_LOGGING_CATEGORY(log_natpmp, "natpmp")

namespace librevault {

NATPMPService::NATPMPService(PortMappingService& parent)
    : PortMappingSubService(parent) {}
NATPMPService::~NATPMPService() { stop(); }

bool NATPMPService::is_config_enabled() {
  return Config::get()->getGlobal("natpmp_enabled").toBool();
}

void NATPMPService::start() {
  if (!is_config_enabled()) return;

  int natpmp_ec = initnatpmp(&natpmp, 0, 0);
  qCDebug(log_natpmp) << "initnatpmp() = " << natpmp_ec;

  if (natpmp_ec == 0) add_existing();
}

void NATPMPService::stop() {
  mappings_.clear();
  closenatpmp(&natpmp);
}

void NATPMPService::map(const QString &id,
                        MappingDescriptor descriptor,
                        const QString &description) {
  mappings_[id] = std::make_unique<NATPMPMapping>(*this, id, descriptor);
}

void NATPMPService::unmap(const QString &id) {
  mappings_.erase(id);
}

/* NATPMPMapping */
NATPMPMapping::NATPMPMapping(NATPMPService& parent, QString id,
                             MappingDescriptor descriptor)
    : parent_(parent), id_(id), descriptor_(descriptor) {
  timer_ = new QTimer(this);

  QTimer::singleShot(0, this, &NATPMPMapping::sendPeriodicRequest);
}

NATPMPMapping::~NATPMPMapping() { sendZeroRequest(); }

void NATPMPMapping::sendPeriodicRequest() {
  int natpmp_ec = sendnewportmappingrequest(
      &parent_.natpmp,
      descriptor_.protocol == QAbstractSocket::TcpSocket ? NATPMP_PROTOCOL_TCP
                                                         : NATPMP_PROTOCOL_UDP,
      descriptor_.port, descriptor_.port,
      Config::get()->getGlobal("natpmp_lifetime").toUInt());
  qCDebug(log_natpmp) << "sendnewportmappingrequest() = " << natpmp_ec;

  natpmpresp_t natpmp_resp;
  do {
    natpmp_ec = readnatpmpresponseorretry(&parent_.natpmp, &natpmp_resp);
  } while (natpmp_ec == NATPMP_TRYAGAIN);
  qCDebug(log_natpmp) << "readnatpmpresponseorretry() = " << natpmp_ec;

  int next_request_sec;
  if (natpmp_ec >= 0) {
    parent_.portMapped(id_, natpmp_resp.pnu.newportmapping.mappedpublicport);
    next_request_sec = natpmp_resp.pnu.newportmapping.lifetime;
  } else {
    qCDebug(log_natpmp) << "Could not set up port mapping";
    next_request_sec = Config::get()->getGlobal("natpmp_lifetime").toUInt();
  }

  timer_->setInterval(next_request_sec * 1000);
}

void NATPMPMapping::sendZeroRequest() {
  int natpmp_ec = sendnewportmappingrequest(
      &parent_.natpmp,
      descriptor_.protocol == QAbstractSocket::TcpSocket ? NATPMP_PROTOCOL_TCP
                                                         : NATPMP_PROTOCOL_UDP,
      descriptor_.port, descriptor_.port, 0);
  qCDebug(log_natpmp) << "sendnewportmappingrequest() = " << natpmp_ec;
}

} /* namespace librevault */
