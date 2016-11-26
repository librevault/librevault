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
#include "control/Config.h"
#include "control/StateCollector.h"
#include "util/log.h"
#include <boost/algorithm/string/predicate.hpp>
#include <librevault/crypto/Hex.h>

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
	conn->set_body(Version::current().version_string());
}

void ControlHTTPServer::handle_restart(ControlServer::server::connection_ptr conn, std::smatch matched) {
	conn->set_status(websocketpp::http::status_code::ok);
	ios_.post([this]{cs_.restart_signal();});
}

void ControlHTTPServer::handle_shutdown(ControlServer::server::connection_ptr conn, std::smatch matched) {
	conn->set_status(websocketpp::http::status_code::ok);
	ios_.post([this]{cs_.shutdown_signal();});
}

void ControlHTTPServer::handle_globals_config(ControlServer::server::connection_ptr conn, std::smatch matched) {
	if(conn->get_request().get_method() == "GET" && !matched[1].matched) {
		conn->set_status(websocketpp::http::status_code::ok);
		conn->append_header("Content-Type", "text/x-json");
		conn->set_body(Json::FastWriter().write(Config::get()->export_globals()));
	}else if(conn->get_request().get_method() == "PUT" && !matched[1].matched) {
		conn->set_status(websocketpp::http::status_code::ok);

		Json::Value new_value;
		Json::Reader().parse(conn->get_request_body(), new_value);

		Config::get()->import_globals(new_value);
	}else if(conn->get_request().get_method() == "GET" && matched[1].matched){
		conn->set_status(websocketpp::http::status_code::ok);
		conn->append_header("Content-Type", "text/x-json");
		conn->set_body(Json::FastWriter().write(Config::get()->global_get(matched[1].str())));
	}else if(conn->get_request().get_method() == "PUT" && matched[1].matched){
		conn->set_status(websocketpp::http::status_code::ok);

		Json::Value new_value;
		Json::Reader().parse(conn->get_request_body(), new_value);

		Config::get()->global_set(matched[1].str(), new_value);
	}else if(conn->get_request().get_method() == "DELETE" && matched[1].matched){
		conn->set_status(websocketpp::http::status_code::ok);
		Config::get()->global_unset(matched[1].str());
	}
}

void ControlHTTPServer::handle_folders_config_all(ControlServer::server::connection_ptr conn, std::smatch matched) {
	if(conn->get_request().get_method() == "GET") {
		conn->set_status(websocketpp::http::status_code::ok);
		conn->append_header("Content-Type", "text/x-json");
		conn->set_body(Json::FastWriter().write(Config::get()->export_folders()));
	}
}

void ControlHTTPServer::handle_folders_config_one(ControlServer::server::connection_ptr conn, std::smatch matched) {
	blob folderid = matched[1].str() | crypto::De<crypto::Hex>();
	if(conn->get_request().get_method() == "GET") {
		conn->set_status(websocketpp::http::status_code::ok);
		conn->append_header("Content-Type", "text/x-json");
		conn->set_body(Json::FastWriter().write(Config::get()->folder_get(folderid)));
	}else if(conn->get_request().get_method() == "PUT") {
		conn->set_status(websocketpp::http::status_code::ok);
		Json::Value new_value;
		Json::Reader().parse(conn->get_request_body(), new_value);

		Config::get()->folder_add(new_value);
	}else if(conn->get_request().get_method() == "DELETE") {
		conn->set_status(websocketpp::http::status_code::ok);
		Config::get()->folder_remove(folderid);
	}
}

void ControlHTTPServer::handle_globals_state(ControlServer::server::connection_ptr conn, std::smatch matched) {
	if(conn->get_request().get_method() == "GET") {
		conn->set_status(websocketpp::http::status_code::ok);
		conn->append_header("Content-Type", "text/x-json");
		conn->set_body(Json::FastWriter().write(state_collector_.global_state()));
	}
}

void ControlHTTPServer::handle_folders_state_all(ControlServer::server::connection_ptr conn, std::smatch matched) {
	conn->set_status(websocketpp::http::status_code::ok);
	conn->append_header("Content-Type", "text/x-json");

	conn->set_body(Json::FastWriter().write(state_collector_.folder_state()));
}

void ControlHTTPServer::handle_folders_state_one(ControlServer::server::connection_ptr conn, std::smatch matched) {
	conn->set_status(websocketpp::http::status_code::ok);
	conn->append_header("Content-Type", "text/x-json");

	blob folderid = matched[1].str() | crypto::De<crypto::Hex>();
	conn->set_body(Json::FastWriter().write(state_collector_.folder_state(folderid)));
}

std::string ControlHTTPServer::make_error_body(const std::string& code, const std::string& description) {
	Json::Value error_json;
	error_json["error_code"] = code.empty() ? "UNKNOWN" : code;
	error_json["description"] = description;
	return Json::FastWriter().write(error_json);
}

} /* namespace librevault */
