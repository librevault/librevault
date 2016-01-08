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
#include "src/pch.h"
#pragma once
#include "NodeKey.h"
#include "src/util/parse_url.h"
#include "src/util/Loggable.h"
#include "websocket_config.h"

namespace librevault {

class Exchanger;
class ExchangeGroup;
class P2PDirectory;

class P2PProvider : protected Loggable {
public:
	using server = websocketpp::server<asio_tls>;
	using client = websocketpp::client<asio_tls_client>;

	enum role_type {SERVER, CLIENT};

	P2PProvider(Client& client, Exchanger& exchanger);
	virtual ~P2PProvider();

	void add_node(url node_url, std::shared_ptr<ExchangeGroup> group_ptr);
	void add_node(const tcp_endpoint& node_endpoint, std::shared_ptr<ExchangeGroup> group_ptr);
	void add_node(const tcp_endpoint& node_endpoint, const blob& pubkey, std::shared_ptr<ExchangeGroup> group_ptr);

	void mark_loopback(const tcp_endpoint& endpoint);
	bool is_loopback(const tcp_endpoint& endpoint);
	bool is_loopback(const blob& pubkey);

	// Getters
	const tcp_endpoint& local_endpoint() const {return local_endpoint_;}
	const NodeKey& node_key() const {return node_key_;}
	std::shared_ptr<ssl_context> ssl_ctx() {return ssl_ctx_ptr_;}

	server& ws_server() {return ws_server_;}
	client& ws_client() {return ws_client_;}

private:
	Client& client_;
	Exchanger& exchanger_;
	NodeKey node_key_;

	server ws_server_;
	client ws_client_;

	tcp_endpoint local_endpoint_;
	std::shared_ptr<ssl_context> ssl_ctx_ptr_;

	std::set<tcp_endpoint> loopback_blacklist_;
	std::mutex loopback_blacklist_mtx_;

	std::unordered_map<server::connection_ptr, std::shared_ptr<P2PDirectory>> ws_server_assignment_;
	std::unordered_map<client::connection_ptr, std::shared_ptr<P2PDirectory>> ws_client_assignment_;

	std::shared_ptr<P2PDirectory> dir_ptr_from_hdl(websocketpp::connection_hdl hdl);

	// TLS functions
	std::shared_ptr<ssl_context> make_ssl_ctx();
	blob pubkey_from_cert(X509* x509);

	// Handlers
	void on_tcp_pre_init(websocketpp::connection_hdl hdl, role_type role);
	std::shared_ptr<ssl_context> on_tls_init(websocketpp::connection_hdl hdl);
	bool on_tls_verify(websocketpp::connection_hdl hdl, bool preverified, boost::asio::ssl::verify_context& ctx);    // Not WebSockets callback, but asio::ssl
	bool on_validate(websocketpp::connection_hdl hdl);
	void on_open(websocketpp::connection_hdl hdl);

	void on_message_server(websocketpp::connection_hdl hdl, server::message_ptr message_ptr);
	void on_message_client(websocketpp::connection_hdl hdl, client::message_ptr message_ptr);
	void on_message(std::shared_ptr<P2PDirectory> dir_ptr, const blob& message_raw);

	void on_fail(websocketpp::connection_hdl hdl);
	void on_close(websocketpp::connection_hdl hdl);
};

} /* namespace librevault */
