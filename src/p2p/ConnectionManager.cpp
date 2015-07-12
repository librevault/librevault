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
#include "ConnectionManager.h"
#include "../Session.h"

namespace librevault {
namespace p2p {

ConnectionManager::ConnectionManager(Session& session) :
		log(spdlog::stderr_logger_mt("cm")), session(session), acceptor(session.get_ios()), ssl_ctx(boost::asio::ssl::context::tlsv12) {
	//ssl_ctx.use_certificate_file(session.get_nodekey().get_cert_path().native(), boost::asio::ssl::context::pem);
	//ssl_ctx.use_private_key_file(session.get_nodekey().get_key_path().native(), boost::asio::ssl::context::pem);

	tcp_endpoint bound_endpoint = tcp_endpoint(address::from_string(session.get_options().get<std::string>("net.ip")), session.get_options().get<uint16_t>("net.port"));

	acceptor.open(bound_endpoint.address().is_v6() ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4());
	acceptor.bind(bound_endpoint);
	acceptor.listen();
	log->info() << "Initialized";
	log->info() << "Listening on " << acceptor.local_endpoint();

	start_accept();
}

ConnectionManager::~ConnectionManager() {
}

void ConnectionManager::start_accept() {
	auto socket_ptr = new ssl_socket(session.get_ios(), ssl_ctx);
	acceptor.async_accept(socket_ptr->lowest_layer(), std::bind([this](ssl_socket* socket_ptr, const boost::system::error_code& ec) {
		log->debug() << "TCP connection accepted from: " << socket_ptr->lowest_layer().remote_endpoint();
		start_accept();
	}, socket_ptr, std::placeholders::_1));
}

Connection::Connection(){}

Connection::Connection(ssl_socket* socket_ptr) : socket_ptr(socket_ptr), log(spdlog::stderr_logger_mt("conn")) {
	remote_endpoint = socket_ptr->lowest_layer().remote_endpoint();
	handshake();
}

Connection::~Connection(){}

void Connection::connect(){

}

void Connection::handshake(){
	socket_ptr->set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
	socket_ptr->async_handshake(boost::asio::ssl::stream_base::server, std::bind(&Connection::handle_handshake, this));
}

void Connection::handle_handshake(){
	log->debug() << "SSL connection established with: " << socket_ptr->lowest_layer().remote_endpoint();
}

} /* namespace p2p */
} /* namespace librevault */
