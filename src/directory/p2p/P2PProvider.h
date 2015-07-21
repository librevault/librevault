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
#include "NodeKey.h"

namespace librevault {

class P2PProvider : public AbstractProvider {
	NodeKey node_key_;

	boost::asio::ssl::context ssl_ctx_;
	boost::asio::ip::tcp::acceptor acceptor_;
public:
	P2PProvider(Session& session, DirectoryExchanger& exchanger);
	virtual ~P2PProvider();

	void accept_operation();

	tcp_endpoint local_endpoint() {return acceptor_.local_endpoint();}
	const NodeKey& node_key() const {return node_key_;}
};

class Connection {
	std::shared_ptr<spdlog::logger> log;

	tcp_endpoint remote_endpoint;
	std::unique_ptr<ssl_socket> socket_ptr;

public:
	Connection(tcp_endpoint endpoint);
	Connection(ssl_socket* socket_ptr);
	~Connection();

	void connect();
	void handshake(boost::asio::ssl::stream_base::handshake_type handshake_type);
	void handle_handshake();
};

} /* namespace librevault */
