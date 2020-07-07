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
  explicit MulticastProvider(QByteArray node_id, QObject* parent);
  virtual ~MulticastProvider();

  [[nodiscard]] Endpoint getEndpointV4() const { return endpoint_v4_; }
  [[nodiscard]] Endpoint getEndpointV6() const { return endpoint_v6_; }

  QUdpSocket* getSocketV4() { return socket4_; }
  QUdpSocket* getSocketV6() { return socket6_; }

  inline QByteArray getNodeId() const { return node_id_; }

 private:
  QByteArray node_id_;

  Endpoint endpoint_v4_;
  Endpoint endpoint_v6_;

  QUdpSocket* socket4_;
  QUdpSocket* socket6_;

 private slots:
  void processDatagram(QUdpSocket* socket);
};

}  // namespace librevault
