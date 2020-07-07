#pragma once
#include <QJsonValue>
#include <QObject>
#include <QVariantMap>
#include <unordered_set>

#include "control/FolderParams.h"
#include "control/websocket_config.h"
#include "util/blob.h"
#include "util/log.h"
#include "util/multi_io_context.h"

Q_DECLARE_LOGGING_CATEGORY(log_control_server)

namespace librevault {

class ClientDaemon;
class StateCollector;
class ControlWebsocketServer;
class ControlHTTPServer;

class ControlServer : public QObject {
  Q_OBJECT
  LOG_SCOPE("ControlServer");

 public:
  using server = websocketpp::server<asio_notls>;

  ControlServer(StateCollector& state_collector, QObject* parent);
  virtual ~ControlServer();

  void run() { ios_.start(); }

  bool check_origin(const QString& origin);

 signals:
  void shutdown();
  void restart();

 public slots:
  void notify_global_config_changed(QString key, QVariant state);
  void notifyGlobalStateChanged(QString key, QJsonValue state);
  void notifyFolderStateChanged(QByteArray folderid, QString key, QJsonValue state);

  void notifyFolderAdded(QByteArray folderid, QVariantMap fconfig);
  void notifyFolderRemoved(QByteArray folderid);

 private:
  multi_io_context ios_;

  server ws_server_;

  std::unique_ptr<ControlWebsocketServer> control_ws_server_;
  std::unique_ptr<ControlHTTPServer> control_http_server_;
};

}  // namespace librevault
