/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Connection.h"
#include "P2PProvider.h"
#include "../../Session.h"

namespace librevault {

Connection::Connection(const url& url, Session& session, P2PProvider& provider) :
		log_(spdlog::get("Librevault")),
		session_(session), provider_(provider),

		socket_(std::make_unique<ssl_socket>(session_.ios(), provider_.ssl_ctx())),
		resolver_(session_.ios()),

		remote_url_(url) {
	state_ = RESOLVE;
	role_ = CLIENT;

	log_->debug() << "Initializing outgoing connection to:" << remote_string();
}

Connection::Connection(tcp_endpoint endpoint, Session& session, P2PProvider& provider) :
		log_(spdlog::get("Librevault")),
		session_(session), provider_(provider),

		socket_(std::make_unique<ssl_socket>(session_.ios(), provider_.ssl_ctx())),
		resolver_(session_.ios()),

		remote_endpoint_(std::move(endpoint)) {
	state_ = CONNECT;
	role_ = CLIENT;

	log_->debug() << "Initializing outgoing connection to:" << remote_string();
}

Connection::Connection(std::unique_ptr<ssl_socket>&& socket, Session& session, P2PProvider& provider) :
		log_(spdlog::get("Librevault")),
		session_(session), provider_(provider),

		socket_(std::move(socket)),
		resolver_(session_.ios()),

		remote_endpoint_(socket_->lowest_layer().remote_endpoint()) {
	state_ = HANDSHAKE;
	role_ = SERVER;

	log_->debug() << "Initializing incoming connection to:" << remote_string();
}

Connection::~Connection() {
	log_->debug() << "Connection to " << remote_string() << " removed.";
}

void Connection::establish(establish_handler handler) {
	establish_handler_ = handler;
	switch(state_){
	case RESOLVE: resolve(); break;
	case CONNECT: connect(); break;
	case HANDSHAKE: handshake(); break;
	default: break;
	}
}

void Connection::disconnect(const boost::system::error_code& error) {
	if(disconnect_mutex_.try_lock()){
		socket_ = std::make_unique<ssl_socket>(session_.ios(), provider_.ssl_ctx());
		state_ = CLOSED;
		establish_handler_(CLOSED, error);
	}
}

void Connection::send(blob bytes, send_handler handler) {
	auto buffer_ptr = std::make_shared<blob>(std::move(bytes));
	boost::asio::async_write(*socket_, boost::asio::buffer(*buffer_ptr), std::bind(
			[this](const boost::system::error_code& error, std::size_t bytes_transferred, decltype(buffer_ptr) buffer_ptr, send_handler handler){
				if(error)
					disconnect(error);
				else
					handler();
			},
			std::placeholders::_1,
			std::placeholders::_2,
			buffer_ptr, handler)
	);
}

void Connection::receive(std::shared_ptr<blob> buffer, receive_handler handler) {
	boost::asio::async_read(*socket_, boost::asio::buffer(*buffer), std::bind(
			[this](const boost::system::error_code& error, std::size_t bytes_transferred, std::shared_ptr<std::vector<uint8_t>> buffer, receive_handler handler){
				if(error)
					disconnect(error);
				else
					handler();
			},
			std::placeholders::_1, std::placeholders::_2, buffer, handler)
	);
}

std::string Connection::remote_string() const {
	if(!remote_url_.host.empty()){
		return remote_url_;
	}else{
		std::ostringstream os;
		os << remote_endpoint_;
		return os.str();
	}
}

void Connection::resolve() {
	boost::asio::ip::tcp::resolver::query query(remote_url_.host, boost::lexical_cast<std::string>(remote_url_.port));

	resolver_.async_resolve(query, std::bind(&Connection::handle_resolve, this, std::placeholders::_1, std::placeholders::_2));
}

void Connection::handle_resolve(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator iterator) {
	state_ = CONNECT;

	boost::asio::async_connect(socket_->lowest_layer(), iterator, std::bind(
			(void(Connection::*)(const boost::system::error_code&, boost::asio::ip::tcp::resolver::iterator))&Connection::handle_connect,
			this,
			std::placeholders::_1, std::placeholders::_2
			));
}

void Connection::connect() {
	state_ = CONNECT;

	socket_->lowest_layer().open(remote_endpoint_.address().is_v6() ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4());
	socket_->lowest_layer().async_connect(remote_endpoint_, std::bind(
			(void(Connection::*)(const boost::system::error_code&))&Connection::handle_connect, this, std::placeholders::_1));
}

void Connection::handle_connect(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator iterator) {
	if(error)
		disconnect(error);
	else{
		remote_endpoint_ = iterator->endpoint();
		handle_connect(error);
	}
}

void Connection::handle_connect(const boost::system::error_code& error) {
	state_ = HANDSHAKE;

	log_->debug() << "Socket connected to: " << remote_string() << " E: " << error;
	handshake();
}

bool Connection::verify_callback(bool preverified, boost::asio::ssl::verify_context& ctx) {
	char subject_name[256];
	X509* x509 = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	X509_NAME_oneline(X509_get_subject_name(x509), subject_name, 256);

	remote_pubkey_ = pubkey_from_cert(x509);

	log_->debug() << "TLS " << preverified << " " << subject_name << " " << (std::string)crypto::Hex().to(remote_pubkey_);

	return true;
}

void Connection::handshake() {
	log_->debug() << "TLS handshake with: " << remote_string();

	boost::asio::ssl::stream_base::handshake_type handshake_type;

	switch(role_){
	case CLIENT:
		handshake_type = boost::asio::ssl::stream_base::client;
		break;
	case SERVER:
		handshake_type = boost::asio::ssl::stream_base::server;
		break;
	}

	socket_->set_verify_callback(std::bind(&Connection::verify_callback, this, std::placeholders::_1, std::placeholders::_2));
	socket_->set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
	socket_->async_handshake(handshake_type, std::bind(&Connection::handle_handshake, this, std::placeholders::_1));
}

void Connection::handle_handshake(const boost::system::error_code& error) {
	state_ = ESTABLISHED;

	log_->debug() << "TLS connection established with: " << remote_string() << " " << error;

	establish_handler_(ESTABLISHED, error);
}

blob Connection::pubkey_from_cert(X509* x509) {
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

} /* namespace librevault */
