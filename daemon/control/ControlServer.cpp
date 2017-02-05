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
#include "ControlServer.h"
#include "Version.h"
#include "ControlHTTPServer.h"
#include "ControlWebsocketServer.h"
#include "control/Config.h"
#include "util/log.h"
#include "util/parse_url.h"
#include <librevault/crypto/Hex.h>
#include <QJsonObject>

namespace librevault {

ControlServer::ControlServer(StateCollector* state_collector, QObject* parent) : QObject(parent),
                                                                                 ios_("ControlServer") {
	control_ws_server_ = std::make_unique<ControlWebsocketServer>(*this, ws_server_, ios_.ios());
	control_http_server_ = std::make_unique<ControlHTTPServer>(*this, ws_server_, *state_collector, ios_.ios());

	QUrl bind_url;
	bind_url.setScheme("http");
	bind_url.setHost("::");
	bind_url.setPort(Config::get()->global_get("control_listen").asUInt());
	tcp_endpoint local_endpoint_ = tcp_endpoint(address::from_string(bind_url.host().toStdString()), bind_url.port());

	/* WebSockets server initialization */
	// General parameters
	ws_server_.init_asio(&ios_.ios());
	ws_server_.set_reuse_addr(true);
	ws_server_.set_user_agent(Version::current().user_agent().toStdString());

	// Handlers
	ws_server_.set_validate_handler(std::bind(&ControlWebsocketServer::on_validate, control_ws_server_.get(), std::placeholders::_1));
	ws_server_.set_open_handler(std::bind(&ControlWebsocketServer::on_open, control_ws_server_.get(), std::placeholders::_1));
	ws_server_.set_fail_handler(std::bind(&ControlWebsocketServer::on_disconnect, control_ws_server_.get(), std::placeholders::_1));
	ws_server_.set_close_handler(std::bind(&ControlWebsocketServer::on_disconnect, control_ws_server_.get(), std::placeholders::_1));

	ws_server_.set_http_handler(std::bind(&ControlHTTPServer::on_http, control_http_server_.get(), std::placeholders::_1));

	ws_server_.listen(local_endpoint_);
	ws_server_.start_accept();

	// This string is for control client, that launches librevault daemon as its child process.
	// It watches STDOUT for ^\[CONTROL\].*?(http:\/\/.*)$ regexp and connects to the first matched address.
	// So, change it carefully, preserving the compatibility.
	std::cout << "[CONTROL] Librevault Client API is listening at " << bind_url.toString().toStdString() << std::endl;

	//connect(Config::get(), &Config::configChanged, this, &ControlServer::notify_global_config_changed);
}

ControlServer::~ControlServer() {
	LOGFUNC();
	ws_server_.stop_listening();

	control_ws_server_->stop();
	ios_.stop(true);

	control_http_server_.reset();
	control_ws_server_.reset();
	LOGFUNCEND();
}

void ControlServer::notify_global_config_changed(QString key, QJsonValue value) {
	QJsonObject event;
	event["key"] = key;
	event["value"] = value;
	control_ws_server_->send_event("EVENT_GLOBAL_CONFIG_CHANGED", event);
}

void ControlServer::notify_global_state_changed(QString key, QJsonValue state) {
	QJsonObject event;
	event["key"] = key;
	event["value"] = state;
	control_ws_server_->send_event("EVENT_GLOBAL_STATE_CHANGED", event);
}

void ControlServer::notify_folder_state_changed(QByteArray folderid, QString key, QJsonValue state) {
	QJsonObject event;
	event["folderid"] = QString(folderid.toHex());
	event["key"] = key;
	event["value"] = state;
	control_ws_server_->send_event("EVENT_FOLDER_STATE_CHANGED", event);
}

void ControlServer::notify_folder_added(const blob& folderid, Json::Value folder_params) {
	Json::Value event;
	event["folderid"] = crypto::Hex().to_string(folderid);
	event["folder_params"] = folder_params;
	//control_ws_server_->send_event("EVENT_FOLDER_ADDED", event);
}

void ControlServer::notify_folder_removed(const blob& folderid) {
	Json::Value event;
	event["folderid"] = crypto::Hex().to_string(folderid);
	//control_ws_server_->send_event("EVENT_FOLDER_REMOVED", event);
}

bool ControlServer::check_origin(const std::string& origin) {
	// Restrict access by "Origin" header
	if(!origin.empty()) {
		url origin_url(origin);
		if(origin_url.host != "127.0.0.1" && origin_url.host != "::1" && origin_url.host != "localhost")
			return false;
	}
	return true;
}

} /* namespace librevault */
