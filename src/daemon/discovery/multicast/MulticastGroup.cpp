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
#include "MulticastGroup.h"

#include <MulticastDiscovery.pb.h>

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
}

void MulticastGroup::setEnabled(bool enabled) {
  if (!timer_->isActive() && enabled)
    timer_->start();
  else if (timer_->isActive() && !enabled)
    timer_->stop();
}

QByteArray MulticastGroup::get_message() {
  if (message_.isEmpty()) {
    protocol::MulticastDiscovery message;

    // Port
    message.set_port(Config::get()->getGlobal("p2p_listen").toUInt());

    // FolderID
    QByteArray folderid = fgroup_->folderid();
    message.set_folderid(folderid.data(), folderid.size());

    // PeerID
    QByteArray digest = provider_->getDigest();
    message.set_digest(digest.data(), digest.size());

    message_.resize(message.ByteSize());
    message.SerializeToArray(message_.data(), message_.size());
  }
  return message_;
}

void MulticastGroup::sendMulticast(QUdpSocket* socket, const Endpoint& endpoint) {
  if (socket->writeDatagram(get_message(), endpoint.addr, endpoint.port))
    qCDebug(log_multicast) << "===> Multicast message sent to:" << endpoint;
  else
    qCDebug(log_multicast) << "=X=> Multicast message not sent to:" << endpoint << "E:" << socket->errorString();
}

void MulticastGroup::sendMulticasts() {
  sendMulticast(provider_->getSocketV4(), Endpoint(provider_->getAddressV4(), provider_->getPort()));
  sendMulticast(provider_->getSocketV6(), Endpoint(provider_->getAddressV6(), provider_->getPort()));
}

}  // namespace librevault
