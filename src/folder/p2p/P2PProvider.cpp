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
#include "P2PProvider.h"
#include "../../Client.h"
#include "P2PFolder.h"
#include "src/folder/FolderGroup.h"
#include "nat/NATPMPService.h"
#include "discovery/StaticDiscovery.h"
#include "discovery/MulticastDiscovery.h"
#include "discovery/BTTrackerDiscovery.h"

namespace librevault {

P2PProvider::P2PProvider(Client& client) :
		Loggable(client, "P2PProvider"),
		client_(client),
		node_key_(client) {
	init_ws();

	static_discovery_ = std::make_unique<StaticDiscovery>(client_);
	multicast4_ = std::make_unique<MulticastDiscovery4>(client_);
	multicast6_ = std::make_unique<MulticastDiscovery6>(client_);
	bttracker_ = std::make_unique<BTTrackerDiscovery>(client_);

	natpmp_ = std::make_unique<NATPMPService>(client_, *this);
	natpmp_->port_signal.connect(std::bind(&P2PProvider::set_public_port, this, std::placeholders::_1));
}

void P2PProvider::init_ws() {
	// Acceptor initialization
	url bind_url = client_.config().getNet_listen();
	local_endpoint_ = tcp_endpoint(address::from_string(bind_url.host), bind_url.port);

	/* WebSockets server initialization */
	// General parameters
	ws_server_.init_asio(&client_.network_ios());
	ws_server_.set_reuse_addr(true);
	ws_server_.set_user_agent(Version::current().user_agent());
	ws_server_.set_max_message_size(10 * 1024 * 1024);

	// Handlers
	ws_server_.set_tcp_pre_init_handler(std::bind(&P2PProvider::on_tcp_pre_init, this, std::placeholders::_1, SERVER));
	ws_server_.set_tls_init_handler(std::bind(&P2PProvider::on_tls_init, this, std::placeholders::_1));
	ws_server_.set_tcp_post_init_handler(std::bind(&P2PProvider::on_tcp_post_init, this, std::placeholders::_1));
	ws_server_.set_validate_handler(std::bind(&P2PProvider::on_validate, this, std::placeholders::_1));
	ws_server_.set_open_handler(std::bind(&P2PProvider::on_open, this, std::placeholders::_1));
	ws_server_.set_message_handler(std::bind(&P2PProvider::on_message_server, this, std::placeholders::_1, std::placeholders::_2));
	ws_server_.set_fail_handler(std::bind(&P2PProvider::on_disconnect, this, std::placeholders::_1));
	ws_server_.set_close_handler(std::bind(&P2PProvider::on_disconnect, this, std::placeholders::_1));

	ws_server_.listen(local_endpoint_);
	ws_server_.start_accept();

	/* WebSockets client initialization */
	// General parameters
	ws_client_.init_asio(&client_.network_ios());
	ws_client_.set_user_agent(Version::current().user_agent());
	ws_client_.set_max_message_size(10 * 1024 * 1024);

	// Handlers
	ws_client_.set_tcp_pre_init_handler(std::bind(&P2PProvider::on_tcp_pre_init, this, std::placeholders::_1, CLIENT));
	ws_client_.set_tls_init_handler(std::bind(&P2PProvider::on_tls_init, this, std::placeholders::_1));
	ws_client_.set_open_handler(std::bind(&P2PProvider::on_open, this, std::placeholders::_1));
	ws_client_.set_message_handler(std::bind(&P2PProvider::on_message_client, this, std::placeholders::_1, std::placeholders::_2));
	ws_client_.set_fail_handler(std::bind(&P2PProvider::on_disconnect, this, std::placeholders::_1));
	ws_client_.set_close_handler(std::bind(&P2PProvider::on_disconnect, this, std::placeholders::_1));

	log_->info() << log_tag() << "Listening on " << local_endpoint();
}

P2PProvider::~P2PProvider() {}

void P2PProvider::add_node(url node_url, std::shared_ptr<FolderGroup> group_ptr) {
	websocketpp::lib::error_code ec;
	node_url.scheme = "wss";
	node_url.query = "/";
	node_url.query += dir_hash_to_query(group_ptr->hash());

	auto connection_ptr = ws_client_.get_connection(node_url, ec);
	auto p2p_directory_ptr = std::make_shared<P2PFolder>(client_, *this, node_url, connection_ptr, group_ptr);
	ws_client_assignment_.insert(std::make_pair(connection_ptr, p2p_directory_ptr));

	log_->debug() << log_tag() << "Added node " << std::string(node_url);

	ws_client_.connect(connection_ptr);
}

void P2PProvider::add_node(const tcp_endpoint& node_endpoint, std::shared_ptr<FolderGroup> group_ptr) {
	if(!group_ptr->have_p2p_dir(node_endpoint) && !is_loopback(node_endpoint)) {
		url node_url;
		node_url.host = node_endpoint.address().to_string();
		node_url.port = node_endpoint.port();

		add_node(node_url, group_ptr);
	}
}

void P2PProvider::add_node(const tcp_endpoint& node_endpoint, const blob& pubkey, std::shared_ptr<FolderGroup> group_ptr) {
	if(!group_ptr->have_p2p_dir(pubkey) && !is_loopback(pubkey)){
		add_node(node_endpoint, group_ptr);
	}
}

void P2PProvider::mark_loopback(const tcp_endpoint& endpoint) {
	std::unique_lock<std::mutex> lk(loopback_blacklist_mtx_);
	loopback_blacklist_.emplace(endpoint);
	log_->notice() << "Marked " << endpoint << " as loopback";
}

bool P2PProvider::is_loopback(const tcp_endpoint& endpoint) {
	std::unique_lock<std::mutex> lk(loopback_blacklist_mtx_);
	return loopback_blacklist_.find(endpoint) != loopback_blacklist_.end();
}

bool P2PProvider::is_loopback(const blob& pubkey) {
	return node_key().public_key() == pubkey;
}

void P2PProvider::set_public_port(uint16_t port) {
	public_port_ = port ? port : local_endpoint().port();
}

std::shared_ptr<P2PFolder> P2PProvider::dir_ptr_from_hdl(websocketpp::connection_hdl hdl) {
	auto conn_server_ptr = ws_server_.get_con_from_hdl(hdl);
	auto conn_client_ptr = ws_client_.get_con_from_hdl(hdl);

	auto it_server = ws_server_assignment_.find(conn_server_ptr);
	auto it_client = ws_client_assignment_.find(conn_client_ptr);

	if(it_server != ws_server_assignment_.end()) {
		return it_server->second;
	}else if(it_client != ws_client_assignment_.end()){
		return it_client->second;
	}else{
		return nullptr;
	}
}

std::shared_ptr<ssl_context> P2PProvider::make_ssl_ctx() {
	auto ssl_ctx_ptr = std::make_shared<ssl_context>(ssl_context::tlsv12);

	// SSL settings
	ssl_ctx_ptr->set_options(
		ssl_context::default_workarounds |
		ssl_context::single_dh_use |
		ssl_context::no_sslv2 |
		ssl_context::no_sslv3 |
		ssl_context::no_tlsv1 |
		ssl_context::no_tlsv1_1
	);

	ssl_ctx_ptr->set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
	ssl_ctx_ptr->use_certificate_file(client_.cert_path().string(), ssl_context::pem);
	ssl_ctx_ptr->use_private_key_file(client_.key_path().string(), ssl_context::pem);
	SSL_CTX_set_cipher_list(ssl_ctx_ptr->native_handle(), "ECDH+ECDSA+AES:!SHA");

	return ssl_ctx_ptr;
}

blob P2PProvider::pubkey_from_cert(X509* x509) {
	std::runtime_error e("Certificate error");

	EC_GROUP* ec_group = EC_GROUP_new_by_curve_name(OBJ_sn2nid("prime256v1"));
	BN_CTX* bn_ctx = BN_CTX_new();

	std::vector<uint8_t> raw_public(33);

	try {
		if(ec_group == NULL || bn_ctx == NULL) throw e;

		EVP_PKEY* remote_pkey = X509_get_pubkey(x509);	if(remote_pkey == NULL) throw e;
		EC_KEY* remote_eckey = EVP_PKEY_get1_EC_KEY(remote_pkey);	if(remote_eckey == NULL) throw e;
		const EC_POINT* remote_pubkey = EC_KEY_get0_public_key(remote_eckey);	if(remote_pubkey == NULL) throw e;

		EC_POINT_point2oct(ec_group, remote_pubkey, POINT_CONVERSION_COMPRESSED, raw_public.data(), raw_public.size(), bn_ctx);
	}catch(...){
		BN_CTX_free(bn_ctx); EC_GROUP_free(ec_group);
		bn_ctx = NULL; ec_group = NULL;

		throw;
	}

	BN_CTX_free(bn_ctx); EC_GROUP_free(ec_group);

	return raw_public;
}

void P2PProvider::on_tcp_pre_init(websocketpp::connection_hdl hdl, role_type role) {
	log_->trace() << log_tag() << "on_tcp_pre_init()";
	log_->debug() << log_tag() << "Opened TCP socket";

	if(role == SERVER) {
		auto connection_ptr = ws_server_.get_con_from_hdl(hdl);
		auto p2p_directory_ptr = std::make_shared<P2PFolder>(client_, *this, connection_ptr->get_remote_endpoint(), connection_ptr);
		ws_server_assignment_.insert(std::make_pair(connection_ptr, p2p_directory_ptr));
	}else{
		auto connection_ptr = ws_client_.get_con_from_hdl(hdl);
		//auto p2p_directory_ptr = ws_client_assignment_[connection_ptr];
		connection_ptr->add_subprotocol("librevault");
	}
}

std::shared_ptr<ssl_context> P2PProvider::on_tls_init(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << "on_tls_init()";

	auto ssl_ctx_ptr = make_ssl_ctx();
	ssl_ctx_ptr->set_verify_callback(std::bind(&P2PProvider::on_tls_verify, this, hdl, std::placeholders::_1, std::placeholders::_2));
	return ssl_ctx_ptr;
}

bool P2PProvider::on_tls_verify(websocketpp::connection_hdl hdl, bool preverified, boost::asio::ssl::verify_context& ctx) {
	log_->trace() << log_tag() << "on_tls_verify()";

	// FIXME: Hey, just returning `true` isn't good enough, yes?
	return true;
}

void P2PProvider::on_tcp_post_init(websocketpp::connection_hdl hdl) {
	auto connection_ptr_client = ws_client_.get_con_from_hdl(hdl);
	auto connection_ptr_server = ws_server_.get_con_from_hdl(hdl);

	struct connection_error{};
	try {
		auto dir_ptr = dir_ptr_from_hdl(hdl);

		// Validate SSL certificate
		X509* x509 = nullptr;
		if(connection_ptr_client)
			x509 = SSL_get_peer_certificate(connection_ptr_client->get_socket().native_handle());
		else
			x509 = SSL_get_peer_certificate(connection_ptr_server->get_socket().native_handle());

		if(x509)
			dir_ptr->remote_pubkey(pubkey_from_cert(x509));
		else
			throw connection_error();

		// Detect loopback
		dir_ptr->update_remote_endpoint();
		if(is_loopback(dir_ptr->remote_pubkey()) || is_loopback(dir_ptr->remote_endpoint())) {
			mark_loopback(dir_ptr->remote_endpoint());
			throw connection_error();
		}
	}catch(connection_error& e){
		if(connection_ptr_client)
			connection_ptr_client->terminate(websocketpp::lib::error_code());
		else if(connection_ptr_server)
			connection_ptr_server->terminate(websocketpp::lib::error_code());
	}
}

bool P2PProvider::on_validate(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << "on_validate()";

	auto connection_ptr = ws_server_.get_con_from_hdl(hdl);
	auto dir_ptr = dir_ptr_from_hdl(hdl);

	// Query validation
	log_->debug() << log_tag() << "Query: " << connection_ptr->get_uri()->get_resource();
	dir_ptr->folder_group_ = client_.get_group(query_to_dir_hash(connection_ptr->get_uri()->get_resource()));

	// Subprotocol management
	auto subprotocols = connection_ptr->get_requested_subprotocols();
	if(std::find(subprotocols.begin(), subprotocols.end(), "librevault") != subprotocols.end()) {
		connection_ptr->select_subprotocol("librevault");
		return true;
	}else
		return false;
}

void P2PProvider::on_open(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << "on_open()";

	auto dir_ptr = dir_ptr_from_hdl(hdl);
	log_->debug() << log_tag() << "Connection opened to: " << dir_ptr->name();
	if(dir_ptr->role_ == CLIENT) {
		dir_ptr->perform_handshake();
	}
}

void P2PProvider::on_message_server(websocketpp::connection_hdl hdl, server::message_ptr message_ptr) {
	log_->trace() << log_tag() << "on_message_server()";

	blob message_raw = blob(std::make_move_iterator(message_ptr->get_payload().begin()), std::make_move_iterator(message_ptr->get_payload().end()));
	on_message(dir_ptr_from_hdl(hdl), message_raw);
}
void P2PProvider::on_message_client(websocketpp::connection_hdl hdl, client::message_ptr message_ptr) {
	log_->trace() << log_tag() << "on_message_client()";

	blob message_raw = blob(std::make_move_iterator(message_ptr->get_payload().begin()), std::make_move_iterator(message_ptr->get_payload().end()));
	on_message(dir_ptr_from_hdl(hdl), message_raw);
}
void P2PProvider::on_message(std::shared_ptr<P2PFolder> dir_ptr, const blob& message_raw) {
	log_->trace() << log_tag() << "on_message()";

	try {
		dir_ptr->handle_message(message_raw);
	}catch(std::runtime_error& e) {
		log_->trace() << log_tag() << "on_message e:" << e.what();
		switch(dir_ptr->role_){
			case P2PProvider::SERVER: {
				ws_server_.get_con_from_hdl(dir_ptr->connection_handle_)->close(websocketpp::close::status::protocol_error, e.what());
			} break;
			case P2PProvider::CLIENT: {
				ws_client_.get_con_from_hdl(dir_ptr->connection_handle_)->close(websocketpp::close::status::protocol_error, e.what());
			} break;
		}
	}
}

void P2PProvider::on_disconnect(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << "on_disconnect()";

	auto dir_ptr = dir_ptr_from_hdl(hdl);
	if(dir_ptr) {
		auto folder_group = dir_ptr->folder_group();
		if(folder_group)
			folder_group->detach(dir_ptr);

		ws_server_assignment_.erase(ws_server_.get_con_from_hdl(hdl));
		ws_client_assignment_.erase(ws_client_.get_con_from_hdl(hdl));
	}
}

std::string P2PProvider::dir_hash_to_query(const blob& dir_hash) {
	return crypto::Hex().to_string(dir_hash);
}

blob P2PProvider::query_to_dir_hash(const std::string& query) {
	return query.substr(1) | crypto::De<crypto::Hex>();
}

} /* namespace librevault */
