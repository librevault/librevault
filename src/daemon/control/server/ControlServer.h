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
#include <QVariantMap>
#include <unordered_set>

#include "control/FolderParams.h"
#include "control/websocket_config.h"
#include "util/blob.h"
#include "util/log.h"
#include "util/multi_io_context.h"

Q_DECLARE_LOGGING_CATEGORY(log_control_server)

namespace librevault {

class Client;
class StateCollector;
class ControlWebsocketServer;
class ControlHTTPServer;

class ControlServer : public QObject {
  Q_OBJECT
  LOG_SCOPE("ControlServer");

 public:
  using server = websocketpp::server<asio_notls>;

  ControlServer(StateCollector* state_collector, QObject* parent);
  virtual ~ControlServer();

  void run() { ios_.start(); }

  bool check_origin(const QString& origin);

 signals:
  void shutdown();
  void restart();

 public slots:
  void notify_global_config_changed(QString key, QVariant state);
  void notify_global_state_changed(QString key, QJsonValue state);
  void notify_folder_state_changed(QByteArray folderid, QString key, QJsonValue state);

  void notify_folder_added(QByteArray folderid, QVariantMap fconfig);
  void notify_folder_removed(QByteArray folderid);

 private:
  multi_io_context ios_;

  server ws_server_;

  std::unique_ptr<ControlWebsocketServer> control_ws_server_;
  std::unique_ptr<ControlHTTPServer> control_http_server_;
};

}  // namespace librevault
