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
#include "BTProvider.h"
#include "util/rand.h"
#include <QDataStream>

namespace librevault {

Q_LOGGING_CATEGORY(log_bt, "discovery.bt")

BTProvider::BTProvider(QObject* parent) : GenericProvider(parent) {
  socket_ = new QUdpSocket();
  socket_->bind();

  connect(socket_, &QUdpSocket::readyRead, this, &BTProvider::processDatagram);
  setIDPrefix();
}

void BTProvider::setIDPrefix(QByteArray peer_id_prefix) {
  peer_id_ = peer_id_prefix + getRandomArray(20);
  peer_id_.resize(20);
}

void BTProvider::processDatagram() {
  QHostAddress tracker_addr;
  QByteArray message(65535, 0);
  qint64 datagram_size = socket_->readDatagram(message.data(), message.size(), &tracker_addr);
  message.resize(datagram_size);

  {
    bool ok = false;
    quint32 addr4 = tracker_addr.toIPv4Address(&ok);
    if (ok) tracker_addr = QHostAddress(addr4);
  }

  qCDebug(log_bt) << "Received BitTorrent tracker message from" << tracker_addr;

  if (message.size() < 8) return;

  QDataStream stream(&message, QIODevice::ReadOnly);
  quint32 action = 0, transaction_id = 0;
  stream >> action >> transaction_id;

  if (action == 0) {  // CONNECT
    quint64 connection_id = 0;
    stream >> connection_id;
    emit receivedConnect(transaction_id, connection_id);
  } else if (action == 1) {  // ANNOUNCE
    quint32 interval = 0, leechers = 0, seeders = 0;
    stream >> interval >> leechers >> seeders;
    EndpointList peers;
    if (tracker_addr.protocol() == QAbstractSocket::IPv4Protocol) {
      peers = btcompat::unpackEnpointList4(message.mid(20));
    } else if (tracker_addr.protocol() == QAbstractSocket::IPv6Protocol) {
      peers = btcompat::unpackEnpointList6(message.mid(20));
    }
    emit receivedAnnounce(transaction_id, interval, leechers, seeders, peers);
  } else if (action == 2) {  // SCRAPE
    // no-op
  } else if (action == 3) {  // ERROR
    emit receivedError(transaction_id, message.mid(8));
  }
}

} /* namespace librevault */
