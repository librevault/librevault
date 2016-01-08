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
#include "src/directory/Exchanger.h"
#include "src/directory/ExchangeGroup.h"
#include "src/directory/fs/FSDirectory.h"
#include "src/directory/p2p/P2PDirectory.h"

namespace librevault {

ControlServer::ControlServer(Client& client) :
		Loggable(client, "ControlServer"), client_(client), timer_(client_.ios()) {
	url bind_url = parse_url(client_.config().get<std::string>("control.listen"));
	local_endpoint_ = tcp_endpoint(address::from_string(bind_url.host), bind_url.port);

	/* WebSockets server initialization */
	// General parameters
	ws_server_.init_asio(&client_.ios());
	ws_server_.set_reuse_addr(true);
	ws_server_.set_user_agent(client_.version().user_agent());

	// Handlers
	ws_server_.set_validate_handler(std::bind(&ControlServer::on_validate, this, std::placeholders::_1));
	ws_server_.set_open_handler(std::bind(&ControlServer::on_open, this, std::placeholders::_1));
	ws_server_.set_message_handler(std::bind(&ControlServer::on_message, this, std::placeholders::_1, std::placeholders::_2));
	ws_server_.set_fail_handler(std::bind(&ControlServer::on_fail, this, std::placeholders::_1));
	ws_server_.set_close_handler(std::bind(&ControlServer::on_close, this, std::placeholders::_1));

	ws_server_.listen(local_endpoint_);
	ws_server_.start_accept();

	send_state_json();
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
	ws_server_assignment_.insert({connection_ptr, std::make_shared<ControlConnection>()});

	send_state_json();
}

void ControlServer::on_message(websocketpp::connection_hdl hdl, server::message_ptr message_ptr) {
	log_->trace() << log_tag() << "on_message()";
	blob message_raw = blob(std::make_move_iterator(message_ptr->get_payload().begin()), std::make_move_iterator(message_ptr->get_payload().end()));

	try {
		// handle message somehow
	}catch(std::runtime_error& e) {
		log_->trace() << log_tag() << "on_message e:" << e.what();
		ws_server_.get_con_from_hdl(hdl)->close(websocketpp::close::status::protocol_error, e.what());
	}
}

void ControlServer::on_fail(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << "on_fail()";
	ws_server_assignment_.erase(ws_server_.get_con_from_hdl(hdl));
	send_state_json();
}

void ControlServer::on_close(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << "on_close()";
	ws_server_assignment_.erase(ws_server_.get_con_from_hdl(hdl));
	send_state_json();
}

std::string ControlServer::make_state_json() {
	ptree state_json;

	// ID
	static int state_json_id = 0;
	state_json.put("id", state_json_id++);

	// Folders
	ptree folders_json;
	for(auto folder : client_.exchanger().groups()) {
		ptree folder_json;
		folder_json.put("path", folder->fs_dir()->open_path());
		folder_json.put("secret", (std::string)folder->key());

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

	// result serialization
	std::ostringstream os; boost::property_tree::write_json(os, state_json, false);
	return os.str();
}

void ControlServer::send_state_json(const boost::system::error_code& ec) {
	log_->trace() << log_tag() << "send_state_json()";
	if(ec == boost::asio::error::operation_aborted) return;

	if(send_mutex_.try_lock()) {
		std::unique_lock<decltype(send_mutex_)> maintain_timer_lk(send_mutex_, std::adopt_lock);
		timer_.cancel();

		auto state_json = make_state_json();

		for(auto conn_assignment : ws_server_assignment_) {
			server::connection_ptr conn_ptr = conn_assignment.first;
			conn_ptr->send(state_json);
		}

		timer_.expires_from_now(std::chrono::seconds(10));  // TODO: Replace with value from config.
		timer_.async_wait(std::bind(&ControlServer::send_state_json, this, std::placeholders::_1));
	}
}

} /* namespace librevault */