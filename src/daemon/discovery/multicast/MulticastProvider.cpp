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
#include "MulticastProvider.h"

#include <QLoggingCategory>
#include <QtCore/QCborValue>
#include <QtCore/QJsonDocument>
#include <QtNetwork/QNetworkDatagram>

#include "MulticastGroup.h"
#include "nodekey/NodeKey.h"

Q_LOGGING_CATEGORY(log_multicast, "discovery.multicast")

namespace librevault {

MulticastProvider::MulticastProvider(NodeKey* nodekey, QObject* parent) : QObject(parent), nodekey_(nodekey) {
  // Multicast parameters
  address_v4_ = QHostAddress("239.192.152.144");
  address_v6_ = QHostAddress("ff08::BD02");
  port_ = 28914;
  //

  socket4_ = new QUdpSocket(this);
  socket6_ = new QUdpSocket(this);

  if (!socket4_->bind(QHostAddress::AnyIPv4, port_))
    qCWarning(log_multicast) << "Could not bind MulticastProvider's IPv4 socket: " << socket4_->errorString();
  if (!socket6_->bind(QHostAddress::AnyIPv6, port_))
    qCWarning(log_multicast) << "Could not bind MulticastProvider's IPv6 socket: " << socket6_->errorString();

  if (!socket4_->joinMulticastGroup(address_v4_))
    qCWarning(log_multicast) << "Could not join IPv4 multicast group: " << socket4_->errorString();
  if (!socket6_->joinMulticastGroup(address_v6_))
    qCWarning(log_multicast) << "Could not join IPv6 multicast group: " << socket6_->errorString();

  socket4_->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);
  socket6_->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);

  connect(socket4_, &QUdpSocket::readyRead, this, [=] { processDatagram(socket4_); });
  connect(socket6_, &QUdpSocket::readyRead, this, [=] { processDatagram(socket6_); });
}

MulticastProvider::~MulticastProvider() {
  socket4_->leaveMulticastGroup(address_v4_);
  socket6_->leaveMulticastGroup(address_v6_);
}

QByteArray MulticastProvider::getDigest() const { return nodekey_->digest(); }

void MulticastProvider::processDatagram(QUdpSocket* socket) {
  while (socket->hasPendingDatagrams()) {
    auto datagram = socket->receiveDatagram();
    Endpoint sender(datagram.senderAddress(), datagram.senderPort());

    QCborParserError parse_error{};
    auto message = QCborValue::fromCbor(datagram.data(), &parse_error);

    if (parse_error.error) {
      qCDebug(log_multicast) << "<=X= Malformed multicast message from:" << sender << "E:" << parse_error.errorString();
      return;
    }

    DiscoveryResult result;
    result.source = QStringLiteral("Multicast");
    result.endpoint = Endpoint(sender.addr, message[DISCOVERY_PORT].toInteger());
    result.digest = message[DISCOVERY_PEER_ID].toByteArray();

    QByteArray community_id = message[DISCOVERY_COMMUNITY_ID].toByteArray();
    qCDebug(log_multicast) << "<=== Multicast message received from: " << result.endpoint;

    emit discovered(community_id, result);
  }
}

}  // namespace librevault
