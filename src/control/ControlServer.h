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
#include "src/directory/p2p/websocket_config.h"

namespace librevault {

class Client;

class ControlServer : public Loggable {
public:
	ControlServer(Client& client);
	virtual ~ControlServer();
private:
	struct ControlConnection {};

	using server = websocketpp::server<asio_notls>;
	Client& client_;

	server ws_server_;
	std::unordered_map<server::connection_ptr, std::shared_ptr<ControlConnection>> ws_server_assignment_;

	tcp_endpoint local_endpoint_;

	boost::asio::steady_timer timer_;
	std::mutex send_mutex_;

	bool on_validate(websocketpp::connection_hdl hdl);
	void on_open(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, server::message_ptr message_ptr);
	void on_fail(websocketpp::connection_hdl hdl);
	void on_close(websocketpp::connection_hdl hdl);

	std::string make_control_json();
	ptree make_state_json() const;
	void send_control_json(const boost::system::error_code& ec = boost::system::error_code());

	void handle_control_json(const ptree& control_json);
	void handle_add_folder_json(const ptree& folder_json);
	void handle_remove_folder_json(const ptree& folder_json);
};

} /* namespace librevault */