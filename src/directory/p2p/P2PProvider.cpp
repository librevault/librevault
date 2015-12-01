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
#include "P2PDirectory.h"
#include "../ExchangeGroup.h"
#include "../Exchanger.h"

namespace librevault {

P2PProvider::P2PProvider(Client& client, Exchanger& exchanger) :
		Loggable(client, "P2PProvider"),
		client_(client),
		exchanger_(exchanger),
		node_key_(client),
		ssl_ctx_(boost::asio::ssl::context::tlsv12),
		acceptor_(client.ios()) {
	// SSL settings
	ssl_ctx_.set_options(
		boost::asio::ssl::context::default_workarounds |
		boost::asio::ssl::context::single_dh_use |
		boost::asio::ssl::context::no_sslv2 |
		boost::asio::ssl::context::no_sslv3
	);

	ssl_ctx_.use_certificate_file(client_.cert_path().string(), boost::asio::ssl::context::pem);
	ssl_ctx_.use_private_key_file(client_.key_path().string(), boost::asio::ssl::context::pem);
	SSL_CTX_set_cipher_list(ssl_ctx_.native_handle(), "ECDH-ECDSA-AES256-GCM-SHA384:ECDH-ECDSA-AES256-SHA384:ECDH-ECDSA-AES128-GCM-SHA256:ECDH-ECDSA-AES128-SHA256");
	// FIXME: Add proper encryption, finally!

	// Acceptor initialization
	url bind_url = parse_url(client_.config().get<std::string>("net.listen"));
	tcp_endpoint bound_endpoint(address::from_string(bind_url.host), bind_url.port);

	acceptor_.open(bound_endpoint.address().is_v6() ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(bound_endpoint);
	acceptor_.listen();
	log_->info() << log_tag() << "Listening on " << local_endpoint();

	accept_operation();
}

P2PProvider::~P2PProvider() {}

void P2PProvider::add_node(const url& node_url, std::shared_ptr<ExchangeGroup> group_ptr) {
	auto connection = std::make_unique<Connection>(node_url, client_, *this);
	auto socket = std::make_shared<P2PDirectory>(std::move(connection), group_ptr, client_, exchanger_, *this);
	unattached_connections_.insert(socket);
}

void P2PProvider::add_node(const tcp_endpoint& node_endpoint, std::shared_ptr<ExchangeGroup> group_ptr) {
	if(!group_ptr->have_p2p_dir(node_endpoint) && loopback_blacklist.find(node_endpoint) == loopback_blacklist.end()){
		auto connection = std::make_unique<Connection>(node_endpoint, client_, *this);
		auto socket = std::make_shared<P2PDirectory>(std::move(connection), group_ptr, client_, exchanger_, *this);
		unattached_connections_.insert(socket);
	}
}

void P2PProvider::add_node(const tcp_endpoint& node_endpoint, const blob& pubkey, std::shared_ptr<ExchangeGroup> group_ptr) {
	if(!group_ptr->have_p2p_dir(pubkey)){
		add_node(node_endpoint, group_ptr);
	}
}

void P2PProvider::remove_from_unattached(std::shared_ptr<P2PDirectory> already_attached) {
	unattached_connections_.erase(already_attached);
}

void P2PProvider::mark_loopback(tcp_endpoint endpoint) {
	loopback_blacklist.emplace(std::move(endpoint));
}

void P2PProvider::accept_operation() {
	auto socket_ptr = new ssl_socket(client_.ios(), ssl_ctx_);

	acceptor_.async_accept(socket_ptr->lowest_layer(), std::bind([this](ssl_socket* socket_ptr, const boost::system::error_code& ec) {
		if(ec == boost::asio::error::operation_aborted) return;
		log_->debug() << log_tag() << "TCP connection accepted from: " << socket_ptr->lowest_layer().remote_endpoint();

		auto connection = std::make_unique<Connection>(std::unique_ptr<ssl_socket>(socket_ptr), client_, *this);
		auto accepted_dir = std::make_shared<P2PDirectory>(std::move(connection), client_, exchanger_, *this);
		unattached_connections_.insert(accepted_dir);

		accept_operation();
	}, socket_ptr, std::placeholders::_1));
}

} /* namespace librevault */
