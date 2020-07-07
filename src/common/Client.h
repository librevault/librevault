#pragma once

#define EXIT_RESTART 451

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <memory>

namespace librevault {

/* Components */
class ControlServer;
class Discovery;
class FolderService;
class NodeKey;
class P2PProvider;
class PortMappingService;
class StateCollector;

class Client : public QObject {
  Q_OBJECT

 signals:
  void globalStateChanged(QString key, QJsonValue value);
  void folderStateChanged(QByteArray folderid, QString key, QJsonValue value);
  void folderAdded(QByteArray folderid);
  void folderRemoved(QByteArray folderid);

 public:
  explicit Client(QObject* parent);
  ~Client() override;

  void run();
  StateCollector& stateCollector() { return *state_collector_; }

 private:
  QNetworkAccessManager* nam_;
  StateCollector* state_collector_;
  NodeKey* node_key_;
  PortMappingService* portmanager_;
  Discovery* discovery_;
  FolderService* folder_service_;
  P2PProvider* p2p_provider_;
};

}  // namespace librevault
