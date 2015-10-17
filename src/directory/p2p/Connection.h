/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "../../pch.h"
#include "../Abstract.h"
#include "../../net/parse_url.h"
#include "P2PProvider.h"

namespace librevault {

class Session;
class P2PProvider;
class Connection {
public:
	enum state {	// Next action TODO: Add some states for SOCKS
		RESOLVE,
		CONNECT,
		HANDSHAKE,

		ESTABLISHED,
		CLOSED
	};

	enum role {	// Librevault protocol is symmetric, but TLS isn't.
		CLIENT,
		SERVER
	};

	using establish_handler = std::function<void(state, const boost::system::error_code&)>;
	using send_handler = std::function<void()>;
	using receive_handler = std::function<void()>;

	// Outgoing
	Connection(const url& url, Session& session, P2PProvider& provider);
	Connection(tcp_endpoint endpoint, Session& session, P2PProvider& provider);
	// Incoming
	Connection(std::unique_ptr<ssl_socket>&& socket, Session& session, P2PProvider& provider);

	virtual ~Connection();

	void establish(establish_handler handler);
	void disconnect(const boost::system::error_code& error);

	void send(const blob& bytes, send_handler handler);
	void receive(std::shared_ptr<blob> buffer, receive_handler handler);

	const tcp_endpoint& remote_endpoint() const {return remote_endpoint_;}
	std::string remote_string() const;
	const blob& remote_pubkey() const {return remote_pubkey_;}
	role get_role() const {return role_;}

private:
	state state_;
	role role_;

	std::shared_ptr<spdlog::logger> log_;
	Session& session_;
	P2PProvider& provider_;

	std::unique_ptr<ssl_socket> socket_;
	boost::asio::ip::tcp::resolver resolver_;

	blob remote_pubkey_;

	tcp_endpoint remote_endpoint_;
	url remote_url_;

	// Send queue
	boost::asio::io_service::strand send_strand_;
	std::queue<std::pair<blob, send_handler>> send_queue_;

	// Mutexes
	std::mutex disconnect_mtx_;

	// Handlers
	establish_handler establish_handler_;

	// Handshake steps
	void resolve();
	void handle_resolve(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator iterator);

	void connect();
	void handle_connect(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator iterator);
	void handle_connect(const boost::system::error_code& error);

	bool verify_callback(bool preverified, boost::asio::ssl::verify_context& ctx);
	void handshake();
	void handle_handshake(const boost::system::error_code& error);

	// Send queue writer
	void write_one();

	// Rest functions
	blob pubkey_from_cert(X509* x509);
};

} /* namespace librevault */
