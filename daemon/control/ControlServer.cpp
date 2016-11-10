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
#include "ControlServer.h"
#include "Client.h"
#include "ControlHTTPServer.h"
#include "ControlWebsocketServer.h"
#include "control/Config.h"
#include "discovery/DiscoveryService.h"
#include "discovery/mldht/MLDHTDiscovery.h"
#include "folder/FolderGroup.h"
#include "folder/FolderService.h"
#include "folder/meta/Index.h"
#include "folder/meta/MetaStorage.h"
#include "p2p/P2PFolder.h"
#include "util/log.h"

namespace librevault {

ControlServer::ControlServer(Client& client) :
		client_(client),
		ios_("ControlServer") {
	control_ws_server_ = std::make_unique<ControlWebsocketServer>(client, *this, ws_server_, ios_.ios());
	control_http_server_ = std::make_unique<ControlHTTPServer>(client, *this, ws_server_, ios_.ios());
	url bind_url = parse_url(Config::get()->globals()["control_listen"].asString());
	local_endpoint_ = tcp_endpoint(address::from_string(bind_url.host), bind_url.port);

	/* WebSockets server initialization */
	// General parameters
	ws_server_.init_asio(&ios_.ios());
	ws_server_.set_reuse_addr(true);
	ws_server_.set_user_agent(Version::current().user_agent());

	// Handlers
	ws_server_.set_validate_handler(std::bind(&ControlWebsocketServer::on_validate, control_ws_server_.get(), std::placeholders::_1));
	ws_server_.set_open_handler(std::bind(&ControlWebsocketServer::on_open, control_ws_server_.get(), std::placeholders::_1));
	ws_server_.set_fail_handler(std::bind(&ControlWebsocketServer::on_disconnect, control_ws_server_.get(), std::placeholders::_1));
	ws_server_.set_close_handler(std::bind(&ControlWebsocketServer::on_disconnect, control_ws_server_.get(), std::placeholders::_1));

	ws_server_.set_http_handler(std::bind(&ControlHTTPServer::on_http, control_http_server_.get(), std::placeholders::_1));

	ws_server_.listen(local_endpoint_);
	ws_server_.start_accept();

	// This string is for control client, that launches librevault daemon as its child process.
	// It watches STDOUT for ^\[CONTROL\].*?(http:\/\/.*)$ regexp and connects to the first matched address.
	// So, change it carefully, preserving the compatibility.
	std::cout << "[CONTROL] Librevault Client API is listening at http://" << (std::string)bind_url << std::endl;
}

ControlServer::~ControlServer() {
	LOGFUNC();
	ws_server_.stop_listening();
	ios_.stop();
	control_http_server_.reset();
	control_ws_server_.reset();
	LOGFUNCEND();
}

std::string ControlServer::make_control_json() {
	Json::Value control_json;

	// ID
	static int control_json_id = 0; // Client-wide message id
	control_json["id"] = control_json_id++;
	control_json["globals"] = Config::get()->globals();
	control_json["folders"] = Config::get()->folders();
	control_json["state"] = make_state_json();

	// result serialization
	std::ostringstream os; os << control_json;
	return os.str();
}

bool ControlServer::check_origin(server::connection_ptr conn) {
	// Restrict access by "Origin" header
	auto origin = conn->get_origin();
	if(!origin.empty()) {
		url origin_url(origin);
		if(origin_url.host != "127.0.0.1" && origin_url.host != "::1" && origin_url.host != "localhost")
			return false;
	}
	return true;
}

Json::Value ControlServer::make_state_json() const {
	Json::Value state_json;                         // state_json
	for(auto folder : client_.folder_service_->groups()) {
		Json::Value folder_json;                    /// folder_json

		folder_json["path"] = folder->params().path.string();
		folder_json["secret"] = folder->secret().string();

		// Indexer
		folder_json["is_indexing"] = folder->meta_storage_->is_indexing();

		// Index
		auto index_status = folder->meta_storage_->index->get_status();
		folder_json["file_entries"] = (Json::Value::UInt64)index_status.file_entries;
		folder_json["directory_entries"] = (Json::Value::UInt64)index_status.directory_entries;
		folder_json["symlink_entries"] = (Json::Value::UInt64)index_status.symlink_entries;
		folder_json["deleted_entries"] = (Json::Value::UInt64)index_status.deleted_entries;

		// Peers
		folder_json["peers"] = Json::arrayValue;
		for(auto p2p_peer : folder->p2p_dirs()) {
			Json::Value peer_json;                  //// peer_json

			std::ostringstream os; os << p2p_peer->remote_endpoint();
			peer_json["endpoint"] = os.str();
			peer_json["client_name"] = p2p_peer->client_name();
			peer_json["user_agent"] = p2p_peer->user_agent();

			// Bandwidth
			auto bandwidth_stats = p2p_peer->heartbeat_stats();
			peer_json["up_bandwidth"] = bandwidth_stats.up_bandwidth_;
			peer_json["up_bandwidth_blocks"] = bandwidth_stats.up_bandwidth_;
			peer_json["down_bandwidth"] = bandwidth_stats.down_bandwidth_;
			peer_json["down_bandwidth_blocks"] = bandwidth_stats.down_bandwidth_;
			// Transferred
			peer_json["up_bytes"] = (Json::Value::UInt64)bandwidth_stats.up_bytes_;
			peer_json["up_bytes_blocks"] = (Json::Value::UInt64)bandwidth_stats.up_bytes_blocks_;
			peer_json["down_bytes"] = (Json::Value::UInt64)bandwidth_stats.down_bytes_;
			peer_json["down_bytes_blocks"] = (Json::Value::UInt64)bandwidth_stats.down_bytes_blocks_;

			folder_json["peers"].append(peer_json); //// /peer_json
		}

		state_json["folders"].append(folder_json);  /// /folder_json
	}

	state_json["dht_nodes_count"] = client_.discovery_->mldht_->node_count();

	return state_json;                              // /state_json
}

} /* namespace librevault */
