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
#include <QJsonDocument>
#include <QRegularExpression>

#include "ControlServer.h"
#include "control/websocket_config.h"
#include "util/log.h"

namespace librevault {

class Client;
class ControlServer;

class ControlHTTPServer {
  LOG_SCOPE("ControlHTTPServer");

 public:
  ControlHTTPServer(ControlServer& cs, ControlServer::server& server, StateCollector& state_collector);
  virtual ~ControlHTTPServer();

  void on_http(websocketpp::connection_hdl hdl);

 private:
  ControlServer& cs_;
  ControlServer::server& server_;
  StateCollector& state_collector_;

  using pconn = ControlServer::server::connection_ptr;
  using http_code = websocketpp::http::status_code::value;

  QList<QPair<QRegularExpression, std::function<void(pconn, QRegularExpressionMatch)>>> handlers_;

  // config
  void handle_globals_config(pconn conn, QRegularExpressionMatch match);
  void handle_folders_config_all(pconn conn, QRegularExpressionMatch match);
  void handle_folders_config_one(pconn conn, QRegularExpressionMatch match);

  // state
  void handle_globals_state(pconn conn, QRegularExpressionMatch match);
  void handle_folders_state_all(pconn conn, QRegularExpressionMatch match);
  void handle_folders_state_one(pconn conn, QRegularExpressionMatch match);

  // daemon
  void handle_restart(pconn conn, QRegularExpressionMatch match);
  void handle_shutdown(pconn conn, QRegularExpressionMatch match);
  void handle_version(pconn conn, QRegularExpressionMatch match);

  /* Error handling */
  std::string make_error_body(const std::string& code, const std::string& description);

  // wrappers
  void sendJson(QJsonDocument json, http_code code, pconn conn);
};

}  // namespace librevault
