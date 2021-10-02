#include "MulticastGroup.h"

#include <QLoggingCategory>
#include <QtCore/QCborMap>
#include <QtCore/QCborValue>

#include "MulticastProvider.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"

Q_DECLARE_LOGGING_CATEGORY(log_multicast)

namespace librevault {

MulticastGroup::MulticastGroup(MulticastProvider* provider, QByteArray groupid, QObject* parent)
    : QObject(parent), provider_(provider), groupid_(std::move(groupid)) {
  timer_ = new QTimer(this);
  timer_->setInterval(Config::get()->getGlobal("multicast_repeat_interval").toInt() * 1000);
  timer_->setTimerType(Qt::VeryCoarseTimer);

  // Connecting signals
  QTimer::singleShot(0, this, &MulticastGroup::sendMulticasts);
  connect(timer_, &QTimer::timeout, this, &MulticastGroup::sendMulticasts);
}

void MulticastGroup::setEnabled(bool enabled) {
  if (!timer_->isActive() && enabled)
    timer_->start();
  else if (timer_->isActive() && !enabled)
    timer_->stop();
}

QByteArray MulticastGroup::getMessage() {
  return !message_.isEmpty()
             ? message_
             : message_ = QJsonDocument(QJsonObject{{DISCOVERY_PORT, (int)Config::get()->getGlobal("p2p_listen").toUInt()},
                                       {DISCOVERY_PEER_ID, QString::fromLatin1(provider_->getNodeId().toHex())},
                                       {DISCOVERY_COMMUNITY_ID, QString::fromLatin1(groupid_.toHex())}}).toJson();
}

void MulticastGroup::sendMulticast(QUdpSocket* socket, const Endpoint& endpoint) {
  if (socket->writeDatagram(getMessage(), endpoint.addr, endpoint.port))
    qCDebug(log_multicast) << "===> Multicast message sent to:" << endpoint;
  else
    qCDebug(log_multicast) << "=X=> Multicast message not sent to:" << endpoint << "E:" << socket->errorString();
}

void MulticastGroup::sendMulticasts() {
  sendMulticast(provider_->getSocketV4(), provider_->getEndpointV4());
  sendMulticast(provider_->getSocketV6(), provider_->getEndpointV6());
}

}  // namespace librevault
