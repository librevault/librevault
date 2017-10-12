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
#include "../btcompat.h"
#include <QHostInfo>
#include <QLoggingCategory>
#include <QTimer>
#include <QUdpSocket>

Q_DECLARE_LOGGING_CATEGORY(log_dht)

namespace librevault {

class Discovery;
class DHTWrapper;
class DHTProvider : public GenericProvider {
  Q_OBJECT

 signals:
  void discovered(QByteArray ih, Endpoint endpoint);
  void nodeCountChanged(int count);

 public:
  explicit DHTProvider(QObject* parent);

  int getNodeCount() const;
  QList<Endpoint> getNodes();
  bool isBound() { return socket4_->isValid() || socket6_->isValid(); }

  Q_SLOT void addRouter(QString host, quint16 port);
  Q_SLOT void addNode(QHostAddress addr, quint16 port);
  Q_SLOT void setPort(quint16 port) { port_ = port; }

  // internal
  void startAnnounce(QByteArray id, QAbstractSocket::NetworkLayerProtocol af, quint16 port);
  void startSearch(QByteArray id, QAbstractSocket::NetworkLayerProtocol af);

 protected:
  void start() override;
  void stop() override;

 private:
  DHTWrapper* dht_wrapper_ = nullptr;

  // Sockets
  QUdpSocket* socket4_;
  QUdpSocket* socket6_;

  quint16 port_ = 0;

  // Initialization
  void readSession(QIODevice* io);
  void writeSession(QIODevice* io);

  QMap<int, quint16> resolves_;

  Q_SLOT void handleResolve(const QHostInfo& host);
  Q_SLOT void handleSearch(QByteArray id, QAbstractSocket::NetworkLayerProtocol af,
      QList<QPair<QHostAddress, quint16>> nodes);
};

} /* namespace librevault */
