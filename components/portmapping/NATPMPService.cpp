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

namespace librevault {

Q_LOGGING_CATEGORY(log_natpmp, "portmapping.natpmp")

namespace {
int makeProtocol(QAbstractSocket::SocketType protocol) {
  return protocol == QAbstractSocket::TcpSocket ? NATPMP_PROTOCOL_TCP : NATPMP_PROTOCOL_UDP;
}
}  // namespace

NATPMPService::NATPMPService(QObject* parent) : MappingService(parent) {
  int natpmp_ec = initnatpmp(&natpmp, 0, 0);
  qCDebug(log_natpmp) << "initnatpmp:" << natpmp_ec;

  if (natpmp_ec != 0) return;
}

NATPMPService::~NATPMPService() { closenatpmp(&natpmp); }

MappedPort* NATPMPService::map(const MappingRequest& request) {
  auto mapping = new NATPMPMappedPort(request, this);
  mappings_.emplace(mapping);
  return mapping;
}

/* NATPMPMapping */
NATPMPMappedPort::NATPMPMappedPort(const MappingRequest& mapping, NATPMPService* parent)
    : MappedPort(parent), parent_(parent), mapping_(mapping) {
  timer_ = new QTimer(this);
  QTimer::singleShot(0, this, &NATPMPMappedPort::sendPeriodicRequest);
}

NATPMPMappedPort::~NATPMPMappedPort() { sendZeroRequest(); }

void NATPMPMappedPort::sendPeriodicRequest() {
  int natpmp_ec = sendnewportmappingrequest(parent_->handle(), makeProtocol(mapping_.protocol),
      mapping_.orig_port, mapping_.mapped_port, parent_->defaultLifetime().count());
  qCDebug(log_natpmp) << "sendnewportmappingrequest:" << natpmp_ec;

  natpmpresp_t natpmp_resp;
  do {
    natpmp_ec = readnatpmpresponseorretry(parent_->handle(), &natpmp_resp);
  } while (natpmp_ec == NATPMP_TRYAGAIN);
  qCDebug(log_natpmp) << "readnatpmpresponseorretry:" << natpmp_ec;

  std::chrono::seconds next_request_sec;
  if (natpmp_ec >= 0) {
    mapped_ = true;

    mapping_.mapped_port = natpmp_resp.pnu.newportmapping.mappedpublicport;
    emit portMapped(mapping_.mapped_port);
    next_request_sec = std::chrono::seconds(natpmp_resp.pnu.newportmapping.lifetime);
  } else {
    mapped_ = false;

    qCDebug(log_natpmp) << "Could not set up port mapping";
    next_request_sec = parent_->defaultLifetime();
  }

  timer_->setInterval(next_request_sec);
}

void NATPMPMappedPort::sendZeroRequest() {
  int natpmp_ec = sendnewportmappingrequest(parent_->handle(), makeProtocol(mapping_.protocol),
      mapping_.orig_port, mapping_.mapped_port, 0);
  qCDebug(log_natpmp) << "sendnewportmappingrequest:" << natpmp_ec;
}

}  // namespace librevault
