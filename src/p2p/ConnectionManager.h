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

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "../types.h"

namespace librevault {
class Session;
namespace p2p {

class ConnectionManager {
	boost::asio::ssl::context ssl_ctx;

	std::shared_ptr<spdlog::logger> log;

	Session& session;
	boost::asio::ip::tcp::acceptor acceptor;
public:
	ConnectionManager(Session& session);
	virtual ~ConnectionManager();

	void start_accept();
};

class Connection {
	std::shared_ptr<spdlog::logger> log;

	tcp_endpoint remote_endpoint;
	std::unique_ptr<ssl_socket> socket_ptr;

public:
	Connection();
	Connection(ssl_socket* socket_ptr);
	~Connection();

	void connect();
	void handshake();
	void handle_handshake();
};

} /* namespace p2p */
} /* namespace librevault */
