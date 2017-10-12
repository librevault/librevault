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
#pragma once

#include "../GenericProvider.h"
#include <QHostInfo>
#include <QLoggingCategory>
#include <QTimer>
#include <QUdpSocket>
#include <QUrl>

namespace librevault {

Q_DECLARE_LOGGING_CATEGORY(log_bt)

class BTProvider;
class BTGroup;

// BEP-0015 partial implementation (without scrape mechanism)
class BTConnection : public HierarchicalService {
  Q_OBJECT

 public:
  BTConnection(const QUrl& tracker_address, BTGroup* btgroup, BTProvider* tracker_provider);
  Q_SIGNAL void discovered(Endpoint endpoint);

 protected:
  void start() override;
  void stop() override;

 private:
  BTProvider* provider_;
  BTGroup* btgroup_;

  // Tracker address
  QUrl tracker_unresolved_;
  Endpoint tracker_resolved_;

  // Connection state
  quint64 conn_id_;
  quint32 transaction_id_;

  // Timers
  QTimer* resolver_timer_;
  QTimer* connect_timer_;
  QTimer* announce_timer_;
  int resolver_lookup_id_ = 0;

  quint32 startTransaction();

  Q_SLOT void resolve();
  Q_SLOT void btconnect();
  Q_SLOT void announce();

  Q_SLOT void handleResolve(const QHostInfo& host);
  Q_SLOT void handleConnect(quint32 transaction_id, quint64 connection_id);
  Q_SLOT void handleAnnounce(quint32 transaction_id, quint32 interval, quint32 leechers, quint32 seeders,
      EndpointList peers);
};

} /* namespace librevault */
