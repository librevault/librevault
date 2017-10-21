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
#include "MulticastGroup.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_multicast, "discovery.multicast")

namespace librevault {

namespace {

QHostAddress makeBindAddress(const QHostAddress& addr) {
  Q_ASSERT(addr.protocol() == QAbstractSocket::IPv4Protocol ||
           addr.protocol() == QAbstractSocket::IPv6Protocol);

  return addr.protocol() == QAbstractSocket::IPv4Protocol ? QHostAddress::AnyIPv4
                                                          : QHostAddress::AnyIPv6;
}

}  // namespace

MulticastProvider::MulticastProvider(QObject* parent) : GenericProvider(parent) {
  socket_ = new QUdpSocket(this);
  connect(socket_, &QUdpSocket::readyRead, [=] { processDatagram(socket_); });
}

void MulticastProvider::start() {
  Q_ASSERT(group_endpoint_.addr.protocol() == QAbstractSocket::IPv4Protocol ||
           group_endpoint_.addr.protocol() == QAbstractSocket::IPv6Protocol);

  QHostAddress bind_addr = makeBindAddress(group_endpoint_.addr);
  if (!socket_->bind(bind_addr, group_endpoint_.port))
    qCWarning(log_multicast) << "Could not bind MulticastProvider's socket:"
                             << socket_->errorString();

  if (!socket_->joinMulticastGroup(group_endpoint_.addr))
    qCWarning(log_multicast) << "Could not join multicast group:" << socket_->errorString();

  socket_->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);

  started();
}

void MulticastProvider::stop() {
  socket_->leaveMulticastGroup(group_endpoint_.addr);
  socket_->close();
}

void MulticastProvider::processDatagram(QUdpSocket* socket) {
  char datagram_buffer[buffer_size_];
  QHostAddress peer_address;
  quint16 peer_port;
  qint64 datagram_size =
      socket->readDatagram(datagram_buffer, buffer_size_, &peer_address, &peer_port);

  // JSON parsing
  QJsonObject message =
      QJsonDocument::fromJson(QByteArray::fromRawData(datagram_buffer, datagram_size)).object();
  if (!message.isEmpty()) {
    qCDebug(log_multicast) << "<=== Multicast message received from:" << peer_address << ":"
                           << peer_port;

    QByteArray id = QByteArray::fromBase64(message["id"].toString().toUtf8());
    quint16 announce_port = message["port"].toInt();

    emit discovered(id, {peer_address, announce_port});
  } else {
    qCDebug(log_multicast) << "<=X= Malformed multicast message from:" << peer_address << ":"
                           << peer_port;
  }
}

} /* namespace librevault */
