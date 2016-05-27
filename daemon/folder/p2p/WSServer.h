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
#include "WSService.h"
#pragma once
#include "websocket_config.h"

namespace librevault {

class WSServer : public WSService {
public:
	using server = websocketpp::server<asio_tls>;

	WSServer(Client& client, P2PProvider& provider);

	const tcp_endpoint& local_endpoint() const {return local_endpoint_;}

protected:
	server ws_server_;
	tcp_endpoint local_endpoint_;

	/* Handlers */
	void on_tcp_post_init(websocketpp::connection_hdl hdl) override {
		WSService::on_tcp_post_init(ws_server_, hdl);
	}
	bool on_validate(websocketpp::connection_hdl hdl);
	void on_message_internal(websocketpp::connection_hdl hdl, server::message_ptr message_ptr);

	/* Util */
	blob query_to_dir_hash(const std::string& query);

	/* Actions */
	void send_message(websocketpp::connection_hdl hdl, const blob& message) override;
	void close(websocketpp::connection_hdl hdl, const std::string& reason) override {
		WSService::close(ws_server_, hdl, reason);
	}
	std::string errmsg(websocketpp::connection_hdl hdl);
};

} /* namespace librevault */
