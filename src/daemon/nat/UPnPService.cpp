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

#include <QTimer>
#include <QUrl>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QNetworkReply>
#include <utility>

#include "nat/upnp/Igd.h"
#include "util/log.h"

Q_LOGGING_CATEGORY(log_upnp, "upnp")

namespace librevault {

UPnPService::UPnPService(QNetworkAccessManager* nam, PortMappingService& parent)
    : PortMappingSubService(parent), nam_(nam) {
  socket_ = new QUdpSocket(this);
}
UPnPService::~UPnPService() { stop(); }

void UPnPService::start() {
  connect(socket_, &QUdpSocket::readyRead, this, &UPnPService::handleDatagram);
  socket_->bind();

  sendSsdpSearch();
}

void UPnPService::stop() {
  socket_->close();
  for (auto igd : activeIgd())
    for (const auto& mapping : mappings_.values()) igd->sendDeletePortMapping(mapping);
}

void UPnPService::sendSsdpSearch() {
  static const auto IGD_SEARCH = QByteArrayLiteral(
      "M-SEARCH * HTTP/1.1\r\n"
      "HOST: 239.255.255.250:1900\r\n"
      "ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
      "MAN: \"ssdp:discover\"\r\n"
      "MX: 2\r\n\r\n");
  socket_->writeDatagram(IGD_SEARCH, QHostAddress("239.255.255.250"), 1900);
}

void UPnPService::handleDatagram() {
  while (socket_->hasPendingDatagrams()) {
    QNetworkDatagram datagram = socket_->receiveDatagram();

    QString usn;

    auto igd = new upnp::Igd(nam_, this);
    igd->igd_addr = datagram.senderAddress();
    igd->local_addr = datagram.destinationAddress();

    connect(igd, &upnp::Igd::ready, this, [=, this] {
      for (const auto& mapping : mappings_.values()) {
        igd->sendAddPortMapping(mapping);
      }
    });
    connect(igd, &upnp::Igd::portMapped, this, &PortMappingSubService::portMapped);

    // Parse datagram
    for (const auto& line : QString::fromUtf8(datagram.data()).split("\r\n")) {
      auto kv = line.split(": ");
      if (kv.first().toLower() == "location")
        igd->gateway_desc = kv.last();
      else if (kv.first().toLower() == "usn")
        usn = kv.last();
    }

    if (!igd_.contains(usn)) {
      qCDebug(log_upnp) << "Found new IGD:" << igd->igd_addr << usn;
      igd_[usn] = igd;
      igd->fetchIgdDescription();
    }
  }
}

QList<upnp::Igd*> UPnPService::activeIgd() const {
  QList<upnp::Igd*> active_igds;
  for (auto igd : igd_)
    if (igd->isReady()) active_igds.append(igd);
  return active_igds;
}

void UPnPService::map(const MappingRequest& request) {
  mappings_[request.id] = request;
  for (auto igd : activeIgd()) igd->sendAddPortMapping(request);
}

void UPnPService::unmap(const QString& id) {
  for (auto igd : activeIgd()) igd->sendDeletePortMapping(mappings_[id]);
  mappings_.remove(id);
}

}  // namespace librevault
