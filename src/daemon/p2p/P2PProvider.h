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
#include <QObject>
#include <QSet>
#include <QWebSocketServer>

#include "discovery/DiscoveryResult.h"

namespace librevault {

class FolderGroup;
class FolderService;
class NodeKey;
class P2PFolder;
class PortMappingService;
class P2PProvider : public QObject {
  Q_OBJECT
 public:
  P2PProvider(NodeKey* node_key, PortMappingService* port_mapping, FolderService* folder_service, QObject* parent);
  virtual ~P2PProvider();

  QSslConfiguration getSslConfiguration() const;

  /* Loopback detection */
  bool isLoopback(QByteArray digest);

 public slots:
  void handleDiscovered(QByteArray folderid, DiscoveryResult result);

 private:
  NodeKey* node_key_;
  PortMappingService* port_mapping_;
  FolderService* folder_service_;

  QWebSocketServer* server_;

 private slots:
  void handleConnection();
  void handlePeerVerifyError(const QSslError& error);
  void handleServerError(QWebSocketProtocol::CloseCode closeCode);
  void handleSslErrors(const QList<QSslError>& errors);
  void handleAcceptError(QAbstractSocket::SocketError socketError);
};

}  // namespace librevault
