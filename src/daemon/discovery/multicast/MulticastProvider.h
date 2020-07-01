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
