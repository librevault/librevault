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
#include "../../pch.h"
#pragma once
#include "NodeKey.h"
#include "../../net/parse_url.h"
#include "../../util/Loggable.h"

namespace librevault {

class Exchanger;
class ExchangeGroup;
class P2PDirectory;

class P2PProvider : protected Loggable {
public:
	P2PProvider(Client& client, Exchanger& exchanger);
	virtual ~P2PProvider();

	void add_node(const url& node_url, std::shared_ptr<ExchangeGroup> group_ptr);
	void add_node(const tcp_endpoint& node_endpoint, std::shared_ptr<ExchangeGroup> group_ptr);

	void add_node(const tcp_endpoint& node_endpoint, const blob& pubkey, std::shared_ptr<ExchangeGroup> group_ptr);

	void remove_from_unattached(std::shared_ptr<P2PDirectory> already_attached);

	void mark_loopback(tcp_endpoint endpoint);

	// Getters
	tcp_endpoint local_endpoint() const {return acceptor_.local_endpoint();}
	const NodeKey& node_key() const {return node_key_;}
	boost::asio::ssl::context& ssl_ctx() {return ssl_ctx_;}

	bool is_v4() const {return local_endpoint().address().is_v4();};
	bool is_v6() const {return local_endpoint().address().is_v6();};

private:
	Client& client_;
	Exchanger& exchanger_;

	NodeKey node_key_;

	boost::asio::ssl::context ssl_ctx_;
	boost::asio::ip::tcp::acceptor acceptor_;

	std::set<std::shared_ptr<P2PDirectory>> unattached_connections_;
	std::set<tcp_endpoint> loopback_blacklist;

	void accept_operation();
};

} /* namespace librevault */
