/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "WSService.h"
#pragma once
#include "websocket_config.h"

namespace librevault {

class WSServer : public WSService {
	LOG_SCOPE("WSServer");
public:
	using server = websocketpp::server<asio_tls>;

	WSServer(Client& client, P2PProvider& provider, PortMappingService& port_mapping, NodeKey& node_key, FolderService& folder_service);
	virtual ~WSServer();

	tcp_endpoint local_endpoint() const;

protected:
	PortMappingService& port_mapping_;

	mutable server ws_server_;

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
	std::string errmsg(websocketpp::connection_hdl hdl) override;
};

} /* namespace librevault */
