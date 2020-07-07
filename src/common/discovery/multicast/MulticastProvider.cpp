#include "MulticastProvider.h"

#include <QLoggingCategory>
#include <QtCore/QCborValue>
#include <QtCore/QJsonDocument>
#include <QtNetwork/QNetworkDatagram>
#include <utility>

#include "MulticastGroup.h"

Q_LOGGING_CATEGORY(log_multicast, "discovery.multicast")

namespace librevault {

MulticastProvider::MulticastProvider(QByteArray node_id, QObject* parent)
    : QObject(parent), node_id_(std::move(node_id)) {
  // Multicast parameters
  endpoint_v4_ = {"239.192.152.144", 28914};
  endpoint_v6_ = {"ff08::BD02", 28914};
  //

  socket4_ = new QUdpSocket(this);
  socket6_ = new QUdpSocket(this);

  if (!socket4_->bind(QHostAddress::AnyIPv4, endpoint_v4_.port))
    qCWarning(log_multicast) << "Could not bind MulticastProvider's IPv4 socket: " << socket4_->errorString();
  if (!socket6_->bind(QHostAddress::AnyIPv6, endpoint_v6_.port))
    qCWarning(log_multicast) << "Could not bind MulticastProvider's IPv6 socket: " << socket6_->errorString();

  if (!socket4_->joinMulticastGroup(endpoint_v4_.addr))
    qCWarning(log_multicast) << "Could not join IPv4 multicast group: " << socket4_->errorString();
  if (!socket6_->joinMulticastGroup(endpoint_v6_.addr))
    qCWarning(log_multicast) << "Could not join IPv6 multicast group: " << socket6_->errorString();

  socket4_->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);
  socket6_->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);

  connect(socket4_, &QUdpSocket::readyRead, this, [=, this] { processDatagram(socket4_); });
  connect(socket6_, &QUdpSocket::readyRead, this, [=, this] { processDatagram(socket6_); });
}

MulticastProvider::~MulticastProvider() {
  socket4_->leaveMulticastGroup(endpoint_v4_.addr);
  socket6_->leaveMulticastGroup(endpoint_v6_.addr);
}

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
