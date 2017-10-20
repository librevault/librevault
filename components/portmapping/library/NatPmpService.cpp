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
#include "NatPmpService.h"

namespace librevault {

Q_LOGGING_CATEGORY(log_natpmp, "portmapping.natpmp")

namespace {

int makeProtocol(QAbstractSocket::SocketType protocol) {
  return protocol == QAbstractSocket::TcpSocket ? NATPMP_PROTOCOL_TCP : NATPMP_PROTOCOL_UDP;
}

}  // namespace

NatPmpService::NatPmpService(QObject* parent) : GenericNatService(parent) {
  natpmp_ = std::make_unique<natpmp_t>();
}

PortMapping* NatPmpService::createMapping(const MappingRequest& request) {
  return new NatPmpPortMapping(request, this);
}

void NatPmpService::start() {
  error_ = initnatpmp(natpmp_.get(), 0, 0);
  qCDebug(log_natpmp) << "initnatpmp:" << error_;
}

void NatPmpService::stop() {
  closenatpmp(natpmp_.get());
}

NatPmpPortMapping::NatPmpPortMapping(const MappingRequest& request, NatPmpService* parent)
    : PortMapping(request, parent) {
  timer_ = new QTimer(this);
}

NatPmpPortMapping::~NatPmpPortMapping() { unmap(); }

void NatPmpPortMapping::map() {
  enabled_ = true;
  if (!isServiceReady()) return;

  int natpmp_ec = sendnewportmappingrequest(qobject_cast<NatPmpService*>(service_)->natpmp_.get(),
      makeProtocol(request_.protocol), request_.internal_port, request_.external_port,
      request_.ttl.count());
  qCDebug(log_natpmp) << "sendnewportmappingrequest:" << natpmp_ec;

  natpmpresp_t natpmp_resp;
  do {
    natpmp_ec = readnatpmpresponseorretry(
        qobject_cast<NatPmpService*>(service_)->natpmp_.get(), &natpmp_resp);
  } while (natpmp_ec == NATPMP_TRYAGAIN);
  qCDebug(log_natpmp) << "readnatpmpresponseorretry:" << natpmp_ec;

  Duration lifetime_sec;
  if (natpmp_ec >= 0) {
    actual_external_port_ = natpmp_resp.pnu.newportmapping.mappedpublicport;
    lifetime_sec = std::chrono::seconds(natpmp_resp.pnu.newportmapping.lifetime);
  } else {
    qCWarning(log_natpmp) << "Could not set up port mapping";
    actual_external_port_ = 0;
    lifetime_sec = request_.ttl;
  }

  expiration_ = std::chrono::system_clock::now() + lifetime_sec;
  timer_->setInterval(lifetime_sec / 3);

  emit mapped(externalPort(), externalAddress(), expiration());
}

void NatPmpPortMapping::unmap() {
  enabled_ = false;
  if (!isServiceReady() || !isMapped()) return;

  timer_->stop();
  int natpmp_ec = sendnewportmappingrequest(qobject_cast<NatPmpService*>(service_)->natpmp_.get(),
      makeProtocol(request_.protocol), request_.internal_port, actual_external_port_, 0);
  qCDebug(log_natpmp) << "sendnewportmappingrequest:" << natpmp_ec;
}

}  // namespace librevault
