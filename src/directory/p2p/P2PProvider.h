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
#include "../../pch.h"
#pragma once
#include "../Abstract.h"
#include "NodeKey.h"
#include "../../net/parse_url.h"

namespace librevault {

class Exchanger;
class P2PDirectory;
class FSDirectory;

class P2PProvider {
public:
	P2PProvider(Session& session, Exchanger& exchanger);
	virtual ~P2PProvider();

	void add_node(const url& node_url, std::shared_ptr<FSDirectory> directory);
	void add_node(const tcp_endpoint& node_endpoint, std::shared_ptr<FSDirectory> directory);

	void add_node(const tcp_endpoint& node_endpoint, const blob& pubkey, std::shared_ptr<FSDirectory> directory);

	void remove_from_unattached(std::shared_ptr<P2PDirectory> already_attached);

	tcp_endpoint local_endpoint() {return acceptor_.local_endpoint();}
	const NodeKey& node_key() const {return node_key_;}
	boost::asio::ssl::context& ssl_ctx() {return ssl_ctx_;}

private:
	Session& session_;
	Exchanger& exchanger_;
	logger_ptr log_;

	NodeKey node_key_;

	std::set<std::shared_ptr<P2PDirectory>> unattached_connections_;

	boost::asio::ssl::context ssl_ctx_;
	boost::asio::ip::tcp::acceptor acceptor_;

	void init_persistent();
	void accept_operation();
};

} /* namespace librevault */
