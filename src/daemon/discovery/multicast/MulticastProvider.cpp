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
#include "MulticastProvider.h"

#include <MulticastDiscovery.pb.h>

#include <QLoggingCategory>
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

  connect(socket4_, &QUdpSocket::readyRead, this, &MulticastProvider::processDatagram);
  connect(socket6_, &QUdpSocket::readyRead, this, &MulticastProvider::processDatagram);
}

MulticastProvider::~MulticastProvider() {
  socket4_->leaveMulticastGroup(address_v4_);
  socket6_->leaveMulticastGroup(address_v6_);
}

QByteArray MulticastProvider::getDigest() const { return nodekey_->digest(); }

void MulticastProvider::processDatagram() {
  // Choose socket to read
  QUdpSocket* socket = nullptr;
  if (socket4_->hasPendingDatagrams())
    socket = socket4_;
  else if (socket6_->hasPendingDatagrams())
    socket = socket6_;

  Q_ASSERT(socket != nullptr);
  Q_ASSERT(socket->hasPendingDatagrams());

  auto datagram = socket->receiveDatagram();
  Endpoint sender(datagram.senderAddress(), datagram.senderPort());
  auto data = datagram.data();

  // Protobuf parsing
  protocol::MulticastDiscovery message;
  if (message.ParseFromArray(data, data.size())) {
    DiscoveryResult result;
    result.source = QStringLiteral("Multicast");
    result.endpoint = Endpoint(sender.addr, message.port());
    result.digest = QByteArray::fromStdString(message.digest());

    QByteArray folderid = QByteArray(message.folderid().data(), message.folderid().size());
    qCDebug(log_multicast) << "<=== Multicast message received from: " << result.endpoint;

    emit discovered(folderid, result);
  } else {
    qCDebug(log_multicast) << "<=X= Malformed multicast message from: " << sender;
  }
}

} /* namespace librevault */
