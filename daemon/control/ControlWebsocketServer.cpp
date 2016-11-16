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
#include "Client.h"
#include "util/log.h"

namespace librevault {

ControlWebsocketServer::ControlWebsocketServer(Client& client, ControlServer& cs, ControlServer::server& server, io_service& ios) :
		client_(client), cs_(cs), server_(server),
		heartbeat_process_(ios, std::bind(&ControlWebsocketServer::send_heartbeat, this, std::placeholders::_1)), id_(0) {
	heartbeat_process_.invoke_post();
}

ControlWebsocketServer::~ControlWebsocketServer() {}

bool ControlWebsocketServer::on_validate(websocketpp::connection_hdl hdl) {
	LOGFUNC();

	auto connection_ptr = server_.get_con_from_hdl(hdl);
	auto subprotocols = connection_ptr->get_requested_subprotocols();

	LOGD("Incoming connection from " << connection_ptr->get_remote_endpoint() << " Origin: " << connection_ptr->get_origin());
	if(!cs_.check_origin(connection_ptr))
		return false;
	// Detect loopback
	if(std::find(subprotocols.begin(), subprotocols.end(), "librevaultctl1.1") != subprotocols.end())
		connection_ptr->select_subprotocol("librevaultctl1.1");
	return true;
}

void ControlWebsocketServer::on_open(websocketpp::connection_hdl hdl) {
	LOGFUNC();

	auto connection_ptr = server_.get_con_from_hdl(hdl);
	ws_sessions_.insert(connection_ptr);

	heartbeat_process_.invoke_post();
}

void ControlWebsocketServer::on_disconnect(websocketpp::connection_hdl hdl) {
	LOGFUNC();

	ws_sessions_.erase(server_.get_con_from_hdl(hdl));
	heartbeat_process_.invoke_post();
}

void ControlWebsocketServer::send_heartbeat(PeriodicProcess& process) {
	//log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	auto control_json = cs_.make_control_json();

	for(auto session : ws_sessions_)
		session->send(control_json);
	process.invoke_after(std::chrono::seconds(1));
}

void ControlWebsocketServer::send_event(const std::string& type, Json::Value event) {
	Json::Value event_message;
	event_message["id"] = Json::Value::UInt64(++id_);
	event_message["type"] = type;
	event_message["event"] = event;

	for(auto session : ws_sessions_)
		session->send(Json::FastWriter().write(event_message));
}

} /* namespace librevault */
