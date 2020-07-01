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
