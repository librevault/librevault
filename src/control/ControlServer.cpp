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
#include "src/Client.h"
#include "src/folder/FolderGroup.h"
#include "src/folder/fs/FSFolder.h"
#include "src/folder/p2p/P2PFolder.h"

namespace librevault {

ControlServer::ControlServer(Client& client) :
		Loggable(client, "ControlServer"), client_(client), timer_(client_.ios()) {
	url bind_url = client_.config().getControl_listen();
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
	log_->trace() << log_tag() << "on_validate()";

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
	log_->trace() << log_tag() << "on_open()";

	auto connection_ptr = ws_server_.get_con_from_hdl(hdl);
	ws_server_assignment_.insert(connection_ptr);

	send_control_json();
}

void ControlServer::on_message(websocketpp::connection_hdl hdl, server::message_ptr message_ptr) {
	log_->trace() << log_tag() << "on_message()";
	blob message_raw = blob(std::make_move_iterator(message_ptr->get_payload().begin()), std::make_move_iterator(message_ptr->get_payload().end()));

	try {
		log_->trace() << message_ptr->get_payload();
		// Read as control_json;
		std::istringstream is(message_ptr->get_payload());
		ptree control_json;
		boost::property_tree::json_parser::read_json(is, control_json);

		handle_control_json(control_json);
	}catch(std::runtime_error& e) { // FIXME: change to websocketpp exception
		log_->trace() << log_tag() << "on_message e:" << e.what();
		ws_server_.get_con_from_hdl(hdl)->close(websocketpp::close::status::protocol_error, e.what());
	}
}

void ControlServer::on_disconnect(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << "on_fail()";
	ws_server_assignment_.erase(ws_server_.get_con_from_hdl(hdl));
	send_control_json();
}

std::string ControlServer::make_control_json() {
	ptree control_json;

	// ID
	static int control_json_id = 0;
	control_json.put("id", control_json_id++);

	control_json.add_child("config", client_.config().get_ptree());
	control_json.add_child("state", make_state_json());

	// result serialization
	std::ostringstream os; boost::property_tree::write_json(os, control_json, false);
	return os.str();
}

ptree ControlServer::make_state_json() const {
	ptree state_json;
	// Folders
	ptree folders_json;
	for(auto folder : client_.groups()) {
		ptree folder_json;
		folder_json.put("path", folder->fs_dir()->open_path());
		folder_json.put("secret", (std::string)folder->secret());

		// Peers
		ptree peers_json;
		for(auto p2p_peer : folder->p2p_dirs()) {
			ptree peer_json;
			peer_json.put("endpoint", p2p_peer->remote_endpoint());
			peers_json.push_back({"", peer_json});
		}
		folder_json.add_child("peers", peers_json);

		folders_json.push_back({"", folder_json});
	}
	state_json.add_child("folders", folders_json);

	return state_json;
}

void ControlServer::send_control_json(const boost::system::error_code& ec) {
	log_->trace() << log_tag() << "send_control_json()";
	if(ec == boost::asio::error::operation_aborted) return;

	if(send_mutex_.try_lock()) {
		std::unique_lock<decltype(send_mutex_)> maintain_timer_lk(send_mutex_, std::adopt_lock);
		timer_.cancel();

		auto control_json = make_control_json();

		for(auto conn_assignment : ws_server_assignment_) {
			server::connection_ptr conn_ptr = conn_assignment.first;
			conn_ptr->send(control_json);
		}

		timer_.expires_from_now(std::chrono::seconds(1));  // TODO: Replace with value from config.
		timer_.async_wait(std::bind(&ControlServer::send_control_json, this, std::placeholders::_1));
	}
}

void ControlServer::handle_control_json(const ptree& control_json) {
	try {
		std::string command = control_json.get<std::string>("command");
		if(command == "set_config") {
			client_.config().apply_ptree(control_json.get_child("config"));
		}else if(command == "add_folder") {
			handle_add_folder_json(control_json.get_child("folder"));
		}else if(command == "remove_folder") {
			handle_remove_folder_json(control_json);
		}else
			log_->debug() << "Could not handle control JSON: Unknown command: " << command;
	}catch(boost::property_tree::ptree_bad_path& e) {
		log_->debug() << "Could not handle control JSON: " << e.what();
	}
}

void ControlServer::handle_add_folder_json(const ptree& folder_json) {
	Config::FolderConfig folder_conf;
	folder_conf.secret = Secret(folder_json.get<std::string>("secret"));
	folder_conf.open_path = folder_json.get<fs::path>("open_path");

	add_folder_signal(folder_conf);
	send_control_json();
}

void ControlServer::handle_remove_folder_json(const ptree& folder_json) {
	Secret secret = Secret(folder_json.get<std::string>("secret"));
	remove_folder_signal(secret);
	send_control_json();
}

} /* namespace librevault */