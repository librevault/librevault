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
#pragma once
#include "WSService.h"
#include "util/parse_url.h"

namespace librevault {

class FolderGroup;

class WSClient : public WSService {
public:
	using client = websocketpp::client<asio_tls_client>;

	struct ConnectCredentials {
		std::string source;

		librevault::url url;
		tcp_endpoint endpoint;
		blob pubkey;
	};

	WSClient(Client& client, P2PProvider& provider);

	void connect(ConnectCredentials node_credentials, std::shared_ptr<FolderGroup> group_ptr);

	/* Actions */
	void send_message(websocketpp::connection_hdl hdl, const blob& message) override;

private:
	client ws_client_;

	/* Handlers */
	void on_tcp_post_init(websocketpp::connection_hdl hdl) override { WSService::on_tcp_post_init(ws_client_, hdl); }
	void on_message_internal(websocketpp::connection_hdl hdl, client::message_ptr message_ptr);

	/* Util */
	std::string dir_hash_to_query(const blob& dir_hash);

	/* Actions */
	void close(websocketpp::connection_hdl hdl, const std::string& reason) override {
		WSService::close(ws_client_, hdl, reason);
	}
	std::string errmsg(websocketpp::connection_hdl hdl) override;

	bool is_loopback(const ConnectCredentials& node_credentials);
	bool already_have(const ConnectCredentials& node_credentials, std::shared_ptr<FolderGroup> group_ptr);
};

} /* namespace librevault */
