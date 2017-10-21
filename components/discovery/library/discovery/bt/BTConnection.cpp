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
#include "BTConnection.h"
#include "util/rand.h"
#include "BTGroup.h"
#include "BTProvider.h"
#include <QDataStream>

#define RESOLVE_INTERVAL 30
#define RECONNECT_INTERVAL 120
#define ANNOUNCE_INTERVAL 30

namespace librevault {

BTConnection::BTConnection(
    const QUrl& tracker_address, BTGroup* btgroup, BTProvider* tracker_provider)
    : provider_(tracker_provider), btgroup_(btgroup), tracker_unresolved_(tracker_address) {
  // Resolve loop
  resolver_timer_ = new QTimer(this);
  resolver_timer_->setInterval(RESOLVE_INTERVAL * 1000);
  resolver_timer_->setTimerType(Qt::VeryCoarseTimer);
  connect(resolver_timer_, &QTimer::timeout, this, &BTConnection::resolve);

  // Connect loop
  connect_timer_ = new QTimer(this);
  connect_timer_->setInterval(RECONNECT_INTERVAL * 1000);
  connect_timer_->setTimerType(Qt::VeryCoarseTimer);
  connect(connect_timer_, &QTimer::timeout, this, &BTConnection::btconnect);

  // Announcer loop
  announce_timer_ = new QTimer(this);
  announce_timer_->setInterval(ANNOUNCE_INTERVAL * 1000);
  announce_timer_->setTimerType(Qt::VeryCoarseTimer);
  connect(announce_timer_, &QTimer::timeout, this, &BTConnection::announce);

  connect(tracker_provider, &BTProvider::receivedConnect, this, &BTConnection::handleConnect);
  connect(tracker_provider, &BTProvider::receivedAnnounce, this, &BTConnection::handleAnnounce);
  connect(this, &BTConnection::discovered, btgroup_, &BTGroup::discovered);
}

void BTConnection::start() {
  QTimer::singleShot(0, this, &BTConnection::resolve);
  resolver_timer_->start();

  started();
}

void BTConnection::stop() {
  announce_timer_->stop();
  connect_timer_->stop();
  resolver_timer_->stop();
}

void BTConnection::resolve() {
  if (resolver_lookup_id_) {
    QHostInfo::abortHostLookup(resolver_lookup_id_);
    qCWarning(log_bt) << "Could not resolve IP address for:" << tracker_unresolved_.host();
  }

  qCDebug(log_bt) << "Resolving IP address for:" << tracker_unresolved_.host();
  resolver_lookup_id_ =
      QHostInfo::lookupHost(tracker_unresolved_.host(), this, SLOT(handleResolve(QHostInfo)));
}

void BTConnection::btconnect() {
  QByteArray message;
  QDataStream stream(&message, QIODevice::WriteOnly);
  stream.setByteOrder(QDataStream::BigEndian);

  stream << (quint64)0x41727101980ULL;  // protocol_id
  stream << (quint32)0;                 // action
  stream << startTransaction();         // transaction_id

  qCDebug(log_bt) << "===> CONNECT to:" << tracker_unresolved_.host()
                  << "transaction:" << transaction_id_;

  provider_->getSocket()->writeDatagram(message, tracker_resolved_.addr, tracker_resolved_.port);
}

void BTConnection::announce() {
  QByteArray message;
  QDataStream stream(&message, QIODevice::WriteOnly);
  stream.setByteOrder(QDataStream::BigEndian);

  stream << conn_id_;                                // connection_id
  stream << (quint32)1;                              // action
  stream << startTransaction();                      // transaction_id
  stream.writeRawData(btgroup_->getInfoHash(), 20);  // info_hash
  stream.writeRawData(provider_->getPeerId(), 20);   // peer_id
  stream << (quint64)0;                              // downloaded
  stream << (quint64)0;                              // left
  stream << (quint64)0;                              // uploaded
  stream << (quint32)0;                              // event
  stream << (quint32)0;                              // ip
  stream.writeRawData(getRandomArray(4), 4);         // key
  stream << (quint32)-1;                             // num_want
  stream << provider_->getAnnouncePort();            // port

  qCDebug(log_bt) << "===> ANNOUNCE to:" << tracker_unresolved_.host()
                  << "transaction:" << transaction_id_;

  provider_->getSocket()->writeDatagram(message, tracker_resolved_.addr, tracker_resolved_.port);
}

quint32 BTConnection::startTransaction() {
  fillRandomBuf((char*)&transaction_id_, 4);
  return transaction_id_;
}

void BTConnection::handleResolve(const QHostInfo& host) {
  resolver_lookup_id_ = 0;
  if (host.error()) {
    qCDebug(log_bt) << "Could not resolve IP address for" << tracker_unresolved_.host()
                    << "E:" << host.errorString();
    resolve();
  } else {
    tracker_resolved_ = {host.addresses().first(), (quint16)tracker_unresolved_.port(80)};
    qCDebug(log_bt) << "Resolved" << tracker_unresolved_.host() << "as" << tracker_resolved_.addr;

    resolver_timer_->stop();
    QTimer::singleShot(0, this, &BTConnection::btconnect);
    connect_timer_->start();
  }
}

void BTConnection::handleConnect(quint32 transaction_id, quint64 connection_id) {
  if (transaction_id != transaction_id_) return;

  qCDebug(log_bt) << "<=== CONNECT from:" << tracker_unresolved_.host()
                  << "transaction:" << transaction_id;

  conn_id_ = connection_id;
  QTimer::singleShot(0, this, &BTConnection::announce);
  announce_timer_->start();
}

void BTConnection::handleAnnounce(quint32 transaction_id, quint32 interval, quint32 leechers,
    quint32 seeders, EndpointList peers) {
  if (transaction_id != transaction_id_) return;

  qCDebug(log_bt) << "<=== ANNOUNCE from:" << tracker_unresolved_.host()
                  << "transaction:" << transaction_id << "interval:" << interval
                  << "leechers:" << leechers << "seeders:" << seeders << "peers:" << peers;

  for (auto& peer : qAsConst(peers)) emit discovered(peer);

  announce_timer_->setInterval(std::max((quint32)RECONNECT_INTERVAL, interval) * 1000);
}

} /* namespace librevault */
