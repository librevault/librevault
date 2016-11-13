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
#include "util/log_scope.h"
#include "util/network.h"
#include "util/parse_url.h"
#include "p2p/websocket_config.h"
#include "control/FolderParams.h"
#include "util/multi_io_service.h"
#include "util/periodic_process.h"
#include <boost/signals2/signal.hpp>
#include <unordered_set>

namespace librevault {

class Client;
class ControlWebsocketServer;
class ControlHTTPServer;

class ControlServer {
	LOG_SCOPE("ControlServer");
public:
	using server = websocketpp::server<asio_notls>;

	ControlServer(Client& client);
	virtual ~ControlServer();

	void run() {ios_.start(1);}
	std::string make_control_json();

	bool check_origin(server::connection_ptr conn);

	boost::signals2::signal<void(Json::Value)> add_folder_signal;
	boost::signals2::signal<void(blob)> remove_folder_signal;
	boost::signals2::signal<void()> shutdown_signal;
	boost::signals2::signal<void()> restart_signal;
private:

	Client& client_;
	multi_io_service ios_;

	server ws_server_;

	std::unique_ptr<ControlWebsocketServer> control_ws_server_;
	std::unique_ptr<ControlHTTPServer> control_http_server_;

	tcp_endpoint local_endpoint_;

	Json::Value make_state_json() const;

	void dispatch_control_json(const Json::Value& control_json);
	void handle_add_folder_json(const Json::Value& control_json);
	void handle_remove_folder_json(const Json::Value& control_json);
};

} /* namespace librevault */