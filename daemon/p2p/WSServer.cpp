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
#include "WSServer.h"
#include "P2PFolder.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "util/parse_url.h"
#include "nat/PortMappingService.h"
#include <librevault/crypto/Hex.h>

namespace librevault {

WSServer::WSServer(io_service& ios, P2PProvider& provider, PortMappingService& port_mapping, NodeKey& node_key, FolderService& folder_service) : WSService(ios, provider, node_key, folder_service), port_mapping_(port_mapping) {
	// Acceptor initialization
	auto endpoint = tcp_endpoint(address_v6::any(), Config::get()->global_get("p2p_listen").asUInt());

	/* WebSockets server initialization */
	// General parameters
	ws_server_.init_asio(&ios_);
	ws_server_.set_reuse_addr(true);
	ws_server_.set_user_agent(Version::current().user_agent().toStdString());
	ws_server_.set_max_message_size(10 * 1024 * 1024);

	// Handlers
	ws_server_.set_tcp_pre_init_handler(std::bind(&WSServer::on_tcp_pre_init, this, std::placeholders::_1, connection::SERVER));
	ws_server_.set_tls_init_handler(std::bind(&WSServer::on_tls_init, this, std::placeholders::_1));
	ws_server_.set_tcp_post_init_handler(std::bind(&WSServer::on_tcp_post_init, this, std::placeholders::_1));
	ws_server_.set_validate_handler(std::bind(&WSServer::on_validate, this, std::placeholders::_1));
	ws_server_.set_open_handler(std::bind(&WSServer::on_open, this, std::placeholders::_1));
	ws_server_.set_message_handler(std::bind(&WSServer::on_message_internal, this, std::placeholders::_1, std::placeholders::_2));
	ws_server_.set_fail_handler(std::bind(&WSServer::on_disconnect, this, std::placeholders::_1));
	ws_server_.set_close_handler(std::bind(&WSServer::on_disconnect, this, std::placeholders::_1));

	ws_server_.listen(endpoint);
	ws_server_.start_accept();

	LOGI("Listening on " << local_endpoint());

	// Port mapping
	port_mapping_.add_port_mapping("main", {local_endpoint().port(), SOCK_STREAM}, "Librevault");
}

WSServer::~WSServer() {
	port_mapping_.remove_port_mapping("main");
}

tcp_endpoint WSServer::local_endpoint() const {
	boost::system::error_code ec;
	auto local_endpoint = ws_server_.get_local_endpoint(ec);
	if(!ec)
		return local_endpoint;
	else
		throw ec;
}

bool WSServer::on_validate(websocketpp::connection_hdl hdl) {
	LOGFUNC();

	auto connection_ptr = ws_server_.get_con_from_hdl(hdl);

	// Query validation
	LOGD("Query: " << connection_ptr->get_uri()->get_resource());
	ws_assignment_[hdl].hash = query_to_dir_hash(connection_ptr->get_uri()->get_resource());

	// Subprotocol management
	auto subprotocols = connection_ptr->get_requested_subprotocols();
	if(std::find(subprotocols.begin(), subprotocols.end(), subprotocol_) != subprotocols.end()) {
		connection_ptr->select_subprotocol(subprotocol_);
		return true;
	}else
		return false;
}

void WSServer::on_message_internal(websocketpp::connection_hdl hdl, server::message_ptr message_ptr) {
	on_message(hdl, message_ptr->get_payload());
}

blob WSServer::query_to_dir_hash(const std::string& query) {
	return query.substr(1) | crypto::De<crypto::Hex>();
}

void WSServer::send_message(websocketpp::connection_hdl hdl, const blob& message) {
	LOGFUNC();
	ws_server_.get_con_from_hdl(hdl)->send(message.data(), message.size());
}

void WSServer::ping(websocketpp::connection_hdl hdl, std::string message) {
	LOGFUNC();
	ws_server_.get_con_from_hdl(hdl)->ping(message);
}

void WSServer::pong(websocketpp::connection_hdl hdl, std::string message) {
	LOGFUNC();
	ws_server_.get_con_from_hdl(hdl)->pong(message);
}

std::string WSServer::errmsg(websocketpp::connection_hdl hdl) {
	std::ostringstream os;
	os << "websocketpp: "
	   << ws_server_.get_con_from_hdl(hdl)->get_ec().message()
	   << " asio: "
	   << ws_server_.get_con_from_hdl(hdl)->get_transport_ec().message();
	return os.str();
}

} /* namespace librevault */
