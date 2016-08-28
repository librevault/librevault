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
#include "WSClient.h"
#include "Client.h"
#include "P2PFolder.h"
#include "folder/FolderGroup.h"
#include <librevault/crypto/Hex.h>

namespace librevault {

WSClient::WSClient(Client& client, P2PProvider& provider) : WSService(client, provider) {
	/* WebSockets client initialization */
	// General parameters
	ws_client_.init_asio(&client_.network_ios());
	ws_client_.set_user_agent(Version::current().user_agent());
	ws_client_.set_max_message_size(10 * 1024 * 1024);

	// Handlers
	ws_client_.set_tcp_pre_init_handler(std::bind(&WSClient::on_tcp_pre_init, this, std::placeholders::_1, connection::CLIENT));
	ws_client_.set_tls_init_handler(std::bind(&WSClient::on_tls_init, this, std::placeholders::_1));
	ws_client_.set_tcp_post_init_handler(std::bind(&WSClient::on_tcp_post_init, this, std::placeholders::_1));
	ws_client_.set_open_handler(std::bind(&WSClient::on_open, this, std::placeholders::_1));
	ws_client_.set_message_handler(std::bind(&WSClient::on_message_internal, this, std::placeholders::_1, std::placeholders::_2));
	ws_client_.set_fail_handler(std::bind(&WSClient::on_disconnect, this, std::placeholders::_1));
	ws_client_.set_close_handler(std::bind(&WSClient::on_disconnect, this, std::placeholders::_1));
}

void WSClient::connect(ConnectCredentials node_credentials, std::shared_ptr<FolderGroup> group_ptr) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	if(node_credentials.url.empty()) {
		assert(node_credentials.endpoint != tcp_endpoint());    // We have no credentials at all, no way to connect

		if(node_credentials.endpoint.address().is_v6()) node_credentials.url.is_ipv6 = true;
		node_credentials.url.host = node_credentials.endpoint.address().to_string();
		node_credentials.url.port = node_credentials.endpoint.port();
	}
	// Assure, that scheme and query are right
	node_credentials.url.scheme = "wss";
	node_credentials.url.query = "/";
	node_credentials.url.query += dir_hash_to_query(group_ptr->hash());

	// URL is ready
	log_->debug() << log_tag() << "Discovered node: " << (std::string)node_credentials.url << " from " << node_credentials.source;

	if(is_loopback(node_credentials)) { // Check for loopback
		log_->debug() << log_tag() << "Refusing to connect to loopback node: " << (std::string)node_credentials.url;
		return;
	}
	if(already_have(node_credentials, group_ptr)) { // Check if already have this node
		log_->debug() << log_tag() << "Refusing to connect to existing node: " << (std::string)node_credentials.url;
		return;
	}

	// Get connection pointer
	websocketpp::lib::error_code ec;
	auto connection_ptr = ws_client_.get_connection(node_credentials.url, ec);
	if(ec) {
		log_->warn() << log_tag() << "Error connecting to " << (std::string)node_credentials.url;
		return;
	}

	// Assign websocketpp connection_hdl to internal P2PFolder connection
	connection& conn = ws_assignment_[websocketpp::connection_hdl(connection_ptr)];
	conn.hash = group_ptr->hash();

	log_->debug() << log_tag() << "Added node " << std::string(node_credentials.url);

	// Actually connect
	ws_client_.connect(connection_ptr);
}

bool WSClient::is_loopback(const ConnectCredentials& node_credentials) {
	if(!node_credentials.pubkey.empty() && provider_.is_loopback(node_credentials.pubkey))  // Public key based loopback (no false negatives!)
		return true;
	if(node_credentials.endpoint != tcp_endpoint() && provider_.is_loopback(node_credentials.endpoint))    // Endpoint-based loopback (may be false negative)
		return true;
	return false;
}

bool WSClient::already_have(const ConnectCredentials& node_credentials, std::shared_ptr<FolderGroup> group_ptr) {
	if(!node_credentials.pubkey.empty() && group_ptr->have_p2p_dir(node_credentials.pubkey))
		return true;
	if(node_credentials.endpoint != tcp_endpoint() && group_ptr->have_p2p_dir(node_credentials.endpoint))
		return true;
	return false;
}

void WSClient::on_message_internal(websocketpp::connection_hdl hdl, client::message_ptr message_ptr) {
	on_message(hdl, message_ptr->get_payload());
}

std::string WSClient::dir_hash_to_query(const blob& dir_hash) {
	return crypto::Hex().to_string(dir_hash);
}

void WSClient::send_message(websocketpp::connection_hdl hdl, const blob& message) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	ws_client_.get_con_from_hdl(hdl)->send(message.data(), message.size());
}

std::string WSClient::errmsg(websocketpp::connection_hdl hdl) {
	return ws_client_.get_con_from_hdl(hdl)->get_transport_ec().message();
}

} /* namespace librevault */
