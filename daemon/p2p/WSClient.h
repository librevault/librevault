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
#pragma once
#include "WSService.h"
#include "discovery/DiscoveryResult.h"
#include <util/parse_url.h>

namespace librevault {

class FolderGroup;

class WSClient : public WSService {
	LOG_SCOPE("WSClient");
public:
	using client = websocketpp::client<asio_tls_client>;

	WSClient(io_service& ios, P2PProvider& provider, NodeKey& node_key, FolderService& folder_service);
	virtual ~WSClient() {}

	void connect(DiscoveryResult node_credentials, std::weak_ptr<FolderGroup> group_ptr);

	/* Actions */
	void send_message(websocketpp::connection_hdl hdl, const blob& message) override;
	void ping(websocketpp::connection_hdl hdl, std::string message) override;
	void pong(websocketpp::connection_hdl hdl, std::string message) override;

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

	bool is_loopback(const DiscoveryResult& node_credentials);
	bool already_have(const DiscoveryResult& node_credentials, std::shared_ptr<FolderGroup> group_ptr);
};

} /* namespace librevault */
