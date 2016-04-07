/* Copyright (C) 2015-2016 Alexander Shishenko <GamePad64@gmail.com>
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
#include "src/pch.h"
#include "src/util/parse_url.h"
#include "src/util/Loggable.h"
#include "src/folder/p2p/websocket_config.h"
#include "src/control/Config.h"
#include "src/control/FolderParams.h"

namespace librevault {

class Client;

class ControlServer : public Loggable {
public:
	ControlServer(Client& client);
	virtual ~ControlServer();

	boost::signals2::signal<void(FolderParams)> add_folder_signal;
	boost::signals2::signal<void(Secret)> remove_folder_signal;
private:
	using server = websocketpp::server<asio_notls>;
	Client& client_;

	server ws_server_;
	std::unordered_set<server::connection_ptr> ws_server_assignment_;

	tcp_endpoint local_endpoint_;

	boost::asio::steady_timer timer_;
	std::mutex send_mutex_;

	bool on_validate(websocketpp::connection_hdl hdl);
	void on_open(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, server::message_ptr message_ptr);
	void on_disconnect(websocketpp::connection_hdl hdl);

	std::string make_control_json();
	Json::Value make_state_json() const;
	void send_control_json(const boost::system::error_code& ec = boost::system::error_code());

	void handle_control_json(const Json::Value& control_json);
	void handle_add_folder_json(const Json::Value& folder_json);
	void handle_remove_folder_json(const Json::Value& folder_json);
};

} /* namespace librevault */