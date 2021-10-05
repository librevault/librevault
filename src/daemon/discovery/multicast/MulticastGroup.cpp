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
#include "MulticastGroup.h"

#include <QLoggingCategory>

#include "MulticastProvider.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"

Q_DECLARE_LOGGING_CATEGORY(log_multicast)

namespace librevault {

MulticastGroup::MulticastGroup(MulticastProvider* provider, FolderGroup* fgroup)
    : provider_(provider), fgroup_(fgroup) {
  timer_ = new QTimer(this);
  timer_->setInterval(Config::get()->getGlobal("multicast_repeat_interval").toInt() * 1000);
  timer_->setTimerType(Qt::VeryCoarseTimer);

  // Connecting signals
  connect(timer_, &QTimer::timeout, this, &MulticastGroup::sendMulticasts);
  QTimer::singleShot(0, this, &MulticastGroup::sendMulticasts);
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
             : message_ = QJsonDocument(QJsonObject
                                     {{DISCOVERY_PORT, (int)Config::get()->getGlobal("p2p_listen").toUInt()},
                                       {DISCOVERY_PEER_ID, QString::fromLatin1(provider_->getDigest().toHex())},
                                       {DISCOVERY_COMMUNITY_ID, QString::fromLatin1(fgroup_->folderid().toHex())}}).toJson();
}

void MulticastGroup::sendMulticast(QUdpSocket* socket, const Endpoint& endpoint) {
  if (socket->writeDatagram(getMessage(), endpoint.addr, endpoint.port))
    qCDebug(log_multicast) << "===> Multicast message sent to:" << endpoint;
  else
    qCDebug(log_multicast) << "=X=> Multicast message not sent to:" << endpoint << "E:" << socket->errorString();
}

void MulticastGroup::sendMulticasts() {
  sendMulticast(provider_->getSocketV4(), Endpoint(provider_->getAddressV4(), provider_->getPort()));
  sendMulticast(provider_->getSocketV6(), Endpoint(provider_->getAddressV6(), provider_->getPort()));
}

}  // namespace librevault
