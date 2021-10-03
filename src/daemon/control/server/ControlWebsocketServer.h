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
#include <QJsonObject>

#include "ControlServer.h"
#include "control/websocket_config.h"
#include "util/log.h"

namespace librevault {

class Client;
class ControlServer;

class ControlWebsocketServer {
  LOG_SCOPE("ControlWebsocketServer");

 public:
  ControlWebsocketServer(ControlServer& cs, ControlServer::server& server);
  virtual ~ControlWebsocketServer();

  void stop();

  bool on_validate(websocketpp::connection_hdl hdl);
  void on_open(websocketpp::connection_hdl hdl);
  void on_disconnect(websocketpp::connection_hdl hdl);

  //
  void send_event(QString type, QJsonObject event);

 private:
  ControlServer& cs_;
  ControlServer::server& server_;

  std::atomic<uint64_t> id_;

  std::unordered_set<ControlServer::server::connection_ptr> ws_sessions_;
};

}  // namespace librevault
