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
#include "websocket_config.h"

namespace librevault {

class P2PFolder;
class P2PProvider;

class Client;

class WSService : public Loggable {
public:
	WSService(Client& client, P2PProvider& provider);

	/* Actions */
	virtual void send_message(websocketpp::connection_hdl hdl, const blob& message) = 0;

protected:
	struct connection_error{};

	P2PProvider& provider_;
	Client& client_;

	std::map<websocketpp::connection_hdl, std::shared_ptr<P2PFolder>, std::owner_less<websocketpp::connection_hdl>> ws_assignment_;

	static const char* subprotocol_;

	/* TLS functions */
	std::shared_ptr<ssl_context> make_ssl_ctx();
	blob pubkey_from_cert(X509* x509);

	/* Handlers */
	std::shared_ptr<ssl_context> on_tls_init(websocketpp::connection_hdl hdl);
	bool on_tls_verify(websocketpp::connection_hdl hdl, bool preverified, boost::asio::ssl::verify_context& ctx);    // Not WebSockets callback, but asio::ssl
	void on_open(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, const std::string& message);
	void on_disconnect(websocketpp::connection_hdl hdl);

	/* Util */
	virtual std::shared_ptr<P2PFolder> dir_ptr_from_hdl(websocketpp::connection_hdl hdl) = 0;

	/* Actions */
	virtual void close(websocketpp::connection_hdl hdl, const std::string& reason) = 0;
	//virtual void terminate(websocketpp::connection_hdl hdl);
};

} /* namespace librevault */