#include "ControlWebsocketServer.h"

#include <QJsonDocument>

#include "util/log.h"

namespace librevault {

ControlWebsocketServer::ControlWebsocketServer(ControlServer& cs, ControlServer::server& server)
    : cs_(cs), server_(server), id_(0) {}
ControlWebsocketServer::~ControlWebsocketServer() {}

void ControlWebsocketServer::stop() {
  for (auto session : ws_sessions_)
    session->close(websocketpp::close::status::going_away, "Librevault daemon is shutting down");
}

bool ControlWebsocketServer::on_validate(websocketpp::connection_hdl hdl) {
  LOGFUNC();

  auto connection_ptr = server_.get_con_from_hdl(hdl);
  auto subprotocols = connection_ptr->get_requested_subprotocols();

  LOGD("Incoming connection from " << connection_ptr->get_remote_endpoint().c_str()
                                   << " Origin: " << connection_ptr->get_origin().c_str());
  if (!cs_.check_origin(QString::fromStdString(connection_ptr->get_origin()))) return false;
  // Detect loopback
  if (std::find(subprotocols.begin(), subprotocols.end(), "librevaultctl1.1") != subprotocols.end())
    connection_ptr->select_subprotocol("librevaultctl1.1");
  return true;
}

void ControlWebsocketServer::on_open(websocketpp::connection_hdl hdl) {
  LOGFUNC();
  ws_sessions_.insert(server_.get_con_from_hdl(hdl));
}

void ControlWebsocketServer::on_disconnect(websocketpp::connection_hdl hdl) {
  LOGFUNC();
  ws_sessions_.erase(server_.get_con_from_hdl(hdl));
}

void ControlWebsocketServer::send_event(QString type, QJsonObject event) {
  QJsonObject event_o;
  event_o["id"] = double(++id_);
  event_o["type"] = type;
  event_o["event"] = event;

  QJsonDocument event_msg(event_o);

  std::string event_msg_s = event_msg.toJson(QJsonDocument::Compact).toStdString();
  for (auto session : ws_sessions_) session->send(event_msg_s);
}

}  // namespace librevault
