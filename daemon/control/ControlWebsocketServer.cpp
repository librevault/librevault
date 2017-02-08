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
#include "ControlWebsocketServer.h"
#include "util/log.h"
#include <QJsonDocument>

namespace librevault {

ControlWebsocketServer::ControlWebsocketServer(ControlServer& cs, ControlServer::server& server, io_service& ios) :
		cs_(cs), server_(server), id_(0) {}
ControlWebsocketServer::~ControlWebsocketServer() {}

void ControlWebsocketServer::stop() {
	for(auto session : ws_sessions_)
		session->close(websocketpp::close::status::going_away, "Librevault daemon is shutting down");
}

bool ControlWebsocketServer::on_validate(websocketpp::connection_hdl hdl) {
	LOGFUNC();

	auto connection_ptr = server_.get_con_from_hdl(hdl);
	auto subprotocols = connection_ptr->get_requested_subprotocols();

	LOGD("Incoming connection from " << connection_ptr->get_remote_endpoint().c_str() << " Origin: " << connection_ptr->get_origin().c_str());
	if(!cs_.check_origin(connection_ptr->get_origin()))
		return false;
	// Detect loopback
	if(std::find(subprotocols.begin(), subprotocols.end(), "librevaultctl1.1") != subprotocols.end())
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
	for(auto session : ws_sessions_)
		session->send(event_msg_s);
}

} /* namespace librevault */
