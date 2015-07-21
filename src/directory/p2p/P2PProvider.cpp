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
#include "P2PProvider.h"
#include "../../Session.h"
#include "../../net/parse_url.h"

namespace librevault {

P2PProvider::P2PProvider(Session& session, DirectoryExchanger& exchanger) :
		AbstractProvider(session, exchanger),
		acceptor_(session.ios()),
		ssl_ctx_(boost::asio::ssl::context::tlsv12),
		node_key_(session.config_path()/"key.pem", session.config_path()/"cert.pem"){
	// SSL settings
	ssl_ctx_.use_certificate_file(node_key_.cert_path().native(), boost::asio::ssl::context::pem);
	ssl_ctx_.use_private_key_file(node_key_.key_path().native(), boost::asio::ssl::context::pem);

	// Acceptor initialization
	url bind_url = parse_url(session.config().get<std::string>("net.listen"));
	tcp_endpoint bound_endpoint(address::from_string(bind_url.host), bind_url.port);

	acceptor_.open(bound_endpoint.address().is_v6() ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4());
	acceptor_.bind(bound_endpoint);
	acceptor_.listen();
	log_->info() << "Listening on " << local_endpoint();

	accept_operation();
}

P2PProvider::~P2PProvider() {}

void P2PProvider::accept_operation() {
	auto socket_ptr = new ssl_socket(session_.ios(), ssl_ctx_);
	acceptor_.async_accept(socket_ptr->lowest_layer(), std::bind([this](ssl_socket* socket_ptr, const boost::system::error_code& ec) {
		log_->debug() << "TCP connection accepted from: " << socket_ptr->lowest_layer().remote_endpoint();
		accept_operation();
	}, socket_ptr, std::placeholders::_1));
}

Connection::Connection(tcp_endpoint endpoint){

}

Connection::Connection(ssl_socket* socket_ptr) : socket_ptr(socket_ptr), log(spdlog::stderr_logger_mt("conn")) {
	remote_endpoint = socket_ptr->lowest_layer().remote_endpoint();
	handshake(boost::asio::ssl::stream_base::server);
}

Connection::~Connection(){}

void Connection::connect(){

}

void Connection::handshake(boost::asio::ssl::stream_base::handshake_type handshake_type){
	socket_ptr->set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
	socket_ptr->async_handshake(handshake_type, std::bind(&Connection::handle_handshake, this));
}

void Connection::handle_handshake(){
	log->debug() << "TLS connection established with: " << socket_ptr->lowest_layer().remote_endpoint();
}

} /* namespace librevault */
