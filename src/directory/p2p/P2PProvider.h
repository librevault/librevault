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

namespace librevault {

class P2PDirectory;
class P2PProvider : public AbstractProvider {
public:
	P2PProvider(Session& session, Exchanger& exchanger);
	virtual ~P2PProvider();

	tcp_endpoint local_endpoint() {return acceptor_.local_endpoint();}
	const NodeKey& node_key() const {return node_key_;}
	boost::asio::ssl::context& ssl_ctx() {return ssl_ctx_;}

private:
	NodeKey node_key_;

	std::multimap<blob, std::shared_ptr<P2PDirectory>> hash_dir_;
	std::set<std::shared_ptr<P2PDirectory>> accepted_connections_;

	boost::asio::ssl::context ssl_ctx_;
	boost::asio::ip::tcp::acceptor acceptor_;

	void init_persistent();
	void accept_operation();
};

} /* namespace librevault */
