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
#pragma once
#include <QUdpSocket>

#include "discovery/DiscoveryResult.h"

namespace librevault {

static const auto DISCOVERY_PORT = QStringLiteral("port");
static const auto DISCOVERY_PEER_ID = QStringLiteral("peer_id");
static const auto DISCOVERY_COMMUNITY_ID = QStringLiteral("community_id");

class MulticastGroup;
class NodeKey;
class MulticastProvider : public QObject {
  Q_OBJECT

 signals:
  void discovered(QByteArray folderid, DiscoveryResult result);

 public:
  explicit MulticastProvider(NodeKey* nodekey, QObject* parent);
  virtual ~MulticastProvider();

  [[nodiscard]] quint16 getPort() const { return port_; }
  [[nodiscard]] QHostAddress getAddressV4() const { return address_v4_; }
  [[nodiscard]] QHostAddress getAddressV6() const { return address_v6_; }

  QUdpSocket* getSocketV4() { return socket4_; }
  QUdpSocket* getSocketV6() { return socket6_; }

  [[nodiscard]] QByteArray getDigest() const;

 private:
  NodeKey* nodekey_;

  QHostAddress address_v4_;
  QHostAddress address_v6_;
  quint16 port_;

  QUdpSocket* socket4_;
  QUdpSocket* socket6_;

 private slots:
  void processDatagram(QUdpSocket* socket);
};

}  // namespace librevault
