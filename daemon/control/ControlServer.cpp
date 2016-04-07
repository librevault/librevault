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
#include "ControlServer.h"
#include "Client.h"
#include "folder/FolderGroup.h"
#include "folder/fs/FSFolder.h"
#include "folder/p2p/P2PFolder.h"

namespace librevault {

ControlServer::ControlServer(Client& client) :
		Loggable(client, "ControlServer"), client_(client), timer_(client_.ios()) {
	//url bind_url = client_.config().getControl_listen();
	url bind_url = parse_url(Config::get()->client()["control_listen"].asString());
	local_endpoint_ = tcp_endpoint(address::from_string(bind_url.host), bind_url.port);

	/* WebSockets server initialization */
	// General parameters
	ws_server_.init_asio(&client_.ios());
	ws_server_.set_reuse_addr(true);
	ws_server_.set_user_agent(Version::current().user_agent());

	// Handlers
	ws_server_.set_validate_handler(std::bind(&ControlServer::on_validate, this, std::placeholders::_1));
	ws_server_.set_open_handler(std::bind(&ControlServer::on_open, this, std::placeholders::_1));
	ws_server_.set_message_handler(std::bind(&ControlServer::on_message, this, std::placeholders::_1, std::placeholders::_2));
	ws_server_.set_fail_handler(std::bind(&ControlServer::on_disconnect, this, std::placeholders::_1));
	ws_server_.set_close_handler(std::bind(&ControlServer::on_disconnect, this, std::placeholders::_1));

	ws_server_.listen(local_endpoint_);
	ws_server_.start_accept();

	// This string is for control client, that launches librevault daemon as its child process.
	// It watches STDOUT for ^\[CONTROL\].*?(wss?:\/\/.*)$ regexp and connects to the first matched address.
	// So, change it carefully, preserving the compatibility.
	std::cout << "[CONTROL] Librevault Control is listening at ws://" << (std::string)bind_url << std::endl;

	send_control_json();
}

ControlServer::~ControlServer() {}

bool ControlServer::on_validate(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto connection_ptr = ws_server_.get_con_from_hdl(hdl);
	auto subprotocols = connection_ptr->get_requested_subprotocols();
	auto origin = connection_ptr->get_origin();

	log_->debug() << log_tag() << "Incoming connection from " << connection_ptr->get_remote_endpoint() << " Origin: " << origin;
	if(!origin.empty() && origin != "http://127.0.0.1" && origin != "https://127.0.0.1") {  // TODO: Maybe, substitute these with WebUI location?
		return false;
	}

	// Detect loopback
	if(std::find(subprotocols.begin(), subprotocols.end(), "librevaultctl1.0") != subprotocols.end())
		connection_ptr->select_subprotocol("librevaultctl1.0");
	return true;
}
//
void ControlServer::on_open(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto connection_ptr = ws_server_.get_con_from_hdl(hdl);
	ws_server_assignment_.insert(connection_ptr);

	send_control_json();
}

void ControlServer::on_message(websocketpp::connection_hdl hdl, server::message_ptr message_ptr) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	try {
		log_->trace() << message_ptr->get_payload();
		// Read as control_json;
		Json::Value control_json;
		Json::Reader r; r.parse(message_ptr->get_payload(), control_json);

		handle_control_json(control_json);
	}catch(std::runtime_error& e) { // FIXME: change to websocketpp exception
		log_->trace() << log_tag() << "on_message e:" << e.what();
		ws_server_.get_con_from_hdl(hdl)->close(websocketpp::close::status::protocol_error, e.what());
	}
}

void ControlServer::on_disconnect(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	ws_server_assignment_.erase(ws_server_.get_con_from_hdl(hdl));
	send_control_json();
}

std::string ControlServer::make_control_json() {
	Json::Value control_json;

	// ID
	static int control_json_id = 0;
	control_json["id"] = control_json_id++;
	control_json["client"] = Config::get()->client();
	control_json["folders"] = Config::get()->folders();
	control_json["state"] = make_state_json();

	// result serialization
	std::ostringstream os; os << control_json;
	return os.str();
}

Json::Value ControlServer::make_state_json() const {
	Json::Value state_json;                         // state_json
	for(auto folder : client_.groups()) {
		Json::Value folder_json;                    /// folder_json

		folder_json["path"] = folder->fs_dir()->path().string();
		folder_json["secret"] = folder->secret().string();

		// Peers
		for(auto p2p_peer : folder->p2p_dirs()) {
			Json::Value peer_json;                  //// peer_json

			std::ostringstream os; os << p2p_peer->remote_endpoint();
			peer_json["endpoint"] = os.str();
			peer_json["client_name"] = p2p_peer->client_name();

			folder_json["peers"].append(peer_json); //// /peer_json
		}

		state_json["folders"].append(folder_json);  /// /folder_json
	}

	return state_json;                              // /state_json
}

void ControlServer::send_control_json(const boost::system::error_code& ec) {
	//log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	if(ec == boost::asio::error::operation_aborted) return;

	if(send_mutex_.try_lock()) {
		std::unique_lock<decltype(send_mutex_)> maintain_timer_lk(send_mutex_, std::adopt_lock);
		timer_.cancel();

		auto control_json = make_control_json();

		for(auto conn_assignment : ws_server_assignment_)
			conn_assignment->send(control_json);

		timer_.expires_from_now(std::chrono::seconds(1));  // TODO: Replace with value from config.
		timer_.async_wait(std::bind(&ControlServer::send_control_json, this, std::placeholders::_1));
	}
}

void ControlServer::handle_control_json(const Json::Value& control_json) {
	std::string command = control_json["command"].asString();
	if(command == "set_config") {
		//client_.config().apply_ptree(control_json.get_child("config"));
	}else if(command == "add_folder") {
		handle_add_folder_json(control_json["folder"]);
	}else if(command == "remove_folder") {
		handle_remove_folder_json(control_json["folder"]);
	}else
		log_->debug() << "Could not handle control JSON: Unknown command: " << command;
}

void ControlServer::handle_add_folder_json(const Json::Value& folder_json) {
	FolderParams params(folder_json);

	add_folder_signal(params);
	send_control_json();
}

void ControlServer::handle_remove_folder_json(const Json::Value& folder_json) {
	Secret secret = Secret(folder_json["secret"].asString());
	remove_folder_signal(secret);
	send_control_json();
}

} /* namespace librevault */