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
  ~P2PProvider() override;

  QSslConfiguration getSslConfiguration() const;

  /* Loopback detection */
  bool isLoopback(const QByteArray& digest);

 public slots:
  void handleDiscovered(const QByteArray& folderid, const DiscoveryResult& result);

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
