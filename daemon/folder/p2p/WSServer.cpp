/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "WSServer.h"
#include "P2PFolder.h"
#include "Client.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "util/parse_url.h"

namespace librevault {

WSServer::WSServer(Client& client, P2PProvider& provider) : WSService(client, provider) {
	// Acceptor initialization
	url bind_url = url(Config::get()->globals()["p2p_listen"].asString());
	local_endpoint_ = tcp_endpoint(address::from_string(bind_url.host), bind_url.port);

	/* WebSockets server initialization */
	// General parameters
	ws_server_.init_asio(&client_.network_ios());
	ws_server_.set_reuse_addr(true);
	ws_server_.set_user_agent(Version::current().user_agent());
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

	ws_server_.listen(local_endpoint_);
	ws_server_.start_accept();

	log_->info() << log_tag() << "Listening on " << local_endpoint_;
}

bool WSServer::on_validate(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto connection_ptr = ws_server_.get_con_from_hdl(hdl);

	// Query validation
	log_->debug() << log_tag() << "Query: " << connection_ptr->get_uri()->get_resource();
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
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	ws_server_.get_con_from_hdl(hdl)->send(message.data(), message.size());
}

} /* namespace librevault */
