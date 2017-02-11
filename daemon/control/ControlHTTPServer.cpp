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
#include "ControlHTTPServer.h"
#include "Version.h"
#include "control/Config.h"
#include "control/StateCollector.h"
#include "util/log.h"
#include <boost/algorithm/string/predicate.hpp>
#include <librevault/crypto/Hex.h>
#include <QJsonArray>
#include <QJsonDocument>

namespace librevault {

#define ADD_HANDLER(REGEX, HANDLER) \
handlers_.push_back(std::make_pair(std::regex(REGEX), [this](ControlServer::server::connection_ptr conn, std::smatch matched){HANDLER(conn, matched);}))

ControlHTTPServer::ControlHTTPServer(ControlServer& cs, ControlServer::server& server, StateCollector& state_collector_, io_service& ios) :
		cs_(cs), server_(server), state_collector_(state_collector_), ios_(ios) {

	// config
	ADD_HANDLER(R"(^\/v1\/globals(?:\/(\w+?))?\/?$)", handle_globals_config);
	ADD_HANDLER(R"(^\/v1\/folders\/?$)", handle_folders_config_all);
	ADD_HANDLER(R"(^\/v1\/folders\/(?!state)(\w+?)\/?$)", handle_folders_config_one);

	// state
	ADD_HANDLER(R"(^\/v1\/state\/?$)", handle_globals_state);
	ADD_HANDLER(R"(^\/v1\/folders\/state\/?$)", handle_folders_state_all);
	ADD_HANDLER(R"(^\/v1\/folders\/(?!state)(\w+?)\/state\/?$)", handle_folders_state_one);

	// daemon
	ADD_HANDLER(R"(^\/v1\/version\/?$)", handle_version);
	ADD_HANDLER(R"(^\/v1\/restart\/?$)", handle_restart);
	ADD_HANDLER(R"(^\/v1\/shutdown\/?$)", handle_shutdown);
}

ControlHTTPServer::~ControlHTTPServer() {}

void ControlHTTPServer::on_http(websocketpp::connection_hdl hdl) {
	LOGFUNC();

	auto conn = server_.get_con_from_hdl(hdl);

	// CORS
	if(!cs_.check_origin(conn->get_request_header("Origin"))) {
		conn->set_status(websocketpp::http::status_code::forbidden);
		conn->set_body(make_error_body("BAD_ORIGIN", "Origin not allowed"));
		return;
	}

	// URI handlers
	try {
		const std::string& uri = conn->get_request().get_uri();
		std::smatch uri_match;
		for(auto& handler : handlers_) {
			if(std::regex_match(uri, uri_match, handler.first)) {
				handler.second(conn, uri_match);
				break;
			}
		}
		if(uri_match.empty()) {
			conn->set_status(websocketpp::http::status_code::not_implemented);
			conn->set_body(make_error_body("", "Handler is not implemented"));
		}
	}catch(std::exception& e){
		conn->set_status(websocketpp::http::status_code::internal_server_error);
		conn->set_body(make_error_body("", e.what()));
	}
}

void ControlHTTPServer::handle_version(ControlServer::server::connection_ptr conn, std::smatch matched) {
	conn->set_status(websocketpp::http::status_code::ok);
	conn->append_header("Content-Type", "text/plain");
	conn->set_body(Version::current().version_string().toStdString());
}

void ControlHTTPServer::handle_restart(ControlServer::server::connection_ptr conn, std::smatch matched) {
	conn->set_status(websocketpp::http::status_code::ok);
	emit cs_.restart();
}

void ControlHTTPServer::handle_shutdown(ControlServer::server::connection_ptr conn, std::smatch matched) {
	conn->set_status(websocketpp::http::status_code::ok);
	emit cs_.shutdown();
}

void ControlHTTPServer::handle_globals_config(ControlServer::server::connection_ptr conn, std::smatch matched) {
	if(conn->get_request().get_method() == "GET" && !matched[1].matched) {
		conn->set_status(websocketpp::http::status_code::ok);
		conn->append_header("Content-Type", "text/x-json");

		QJsonDocument msg(Config::get()->exportGlobals());
		conn->set_body(msg.toJson(QJsonDocument::Compact).toStdString());
	}else if(conn->get_request().get_method() == "PUT" && !matched[1].matched) {
		conn->set_status(websocketpp::http::status_code::ok);

		QJsonDocument new_config = QJsonDocument::fromJson(QByteArray::fromStdString(conn->get_request_body()));

		Config::get()->importGlobals(new_config);
	}else if(conn->get_request().get_method() == "GET" && matched[1].matched){
		conn->set_status(websocketpp::http::status_code::ok);
		conn->append_header("Content-Type", "text/x-json");

		QJsonObject o;
		o["key"] = QString::fromStdString(matched[1].str());
		o["value"] = QJsonValue::fromVariant(Config::get()->getGlobal(QString::fromStdString(matched[1].str())));

		conn->set_body(QJsonDocument(o).toJson().toStdString());
	}else if(conn->get_request().get_method() == "PUT" && matched[1].matched){
		conn->set_status(websocketpp::http::status_code::ok);

		QJsonObject o = QJsonDocument::fromJson(QByteArray::fromStdString(conn->get_request_body())).object();

		Config::get()->setGlobal(QString::fromStdString(matched[1].str()), o["value"].toVariant());
	}else if(conn->get_request().get_method() == "DELETE" && matched[1].matched){
		conn->set_status(websocketpp::http::status_code::ok);
		Config::get()->removeGlobal(QString::fromStdString(matched[1].str()));
	}
}

void ControlHTTPServer::handle_folders_config_all(ControlServer::server::connection_ptr conn, std::smatch matched) {
	if(conn->get_request().get_method() == "GET") {
		conn->set_status(websocketpp::http::status_code::ok);
		conn->append_header("Content-Type", "text/x-json");

		conn->set_body(QJsonDocument(Config::get()->exportFolders()).toJson().toStdString());
	}
}

void ControlHTTPServer::handle_folders_config_one(ControlServer::server::connection_ptr conn, std::smatch matched) {
	QByteArray folderid = QByteArray::fromHex(QByteArray::fromStdString(matched[1].str()));
	if(conn->get_request().get_method() == "GET") {
		conn->set_status(websocketpp::http::status_code::ok);
		conn->append_header("Content-Type", "text/x-json");
		conn->set_body(QJsonDocument(QJsonObject::fromVariantMap(Config::get()->getFolder(folderid))).toJson().toStdString());
	}else if(conn->get_request().get_method() == "PUT") {
		conn->set_status(websocketpp::http::status_code::ok);
		QJsonObject new_value = QJsonDocument::fromJson(QByteArray::fromStdString(conn->get_request_body())).object();

		Config::get()->addFolder(new_value.toVariantMap());
	}else if(conn->get_request().get_method() == "DELETE") {
		conn->set_status(websocketpp::http::status_code::ok);
		Config::get()->removeFolder(folderid);
	}
}

void ControlHTTPServer::handle_globals_state(ControlServer::server::connection_ptr conn, std::smatch matched) {
	if(conn->get_request().get_method() == "GET") {
		conn->set_status(websocketpp::http::status_code::ok);
		conn->append_header("Content-Type", "text/x-json");

		QJsonDocument msg(state_collector_.global_state());
		conn->set_body(msg.toJson(QJsonDocument::Compact).toStdString());
	}
}

void ControlHTTPServer::handle_folders_state_all(ControlServer::server::connection_ptr conn, std::smatch matched) {
	conn->set_status(websocketpp::http::status_code::ok);
	conn->append_header("Content-Type", "text/x-json");

	QJsonDocument msg(state_collector_.folder_state());
	conn->set_body(msg.toJson(QJsonDocument::Compact).toStdString());
}

void ControlHTTPServer::handle_folders_state_one(ControlServer::server::connection_ptr conn, std::smatch matched) {
	conn->set_status(websocketpp::http::status_code::ok);
	conn->append_header("Content-Type", "text/x-json");

	QByteArray folderid = QByteArray::fromHex(QByteArray::fromStdString(matched[1].str()));

	QJsonDocument msg(state_collector_.folder_state(folderid));
	conn->set_body(msg.toJson(QJsonDocument::Compact).toStdString());
}

std::string ControlHTTPServer::make_error_body(const std::string& code, const std::string& description) {
	QJsonObject error_json;
	error_json["error_code"] = code.empty() ? "UNKNOWN" : QString::fromStdString(code);
	error_json["description"] = QString::fromStdString(description);
	return QJsonDocument(error_json).toJson().toStdString();
}

} /* namespace librevault */
