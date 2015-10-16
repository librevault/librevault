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
#include "P2PDirectory.h"
#include "../fs/FSDirectory.h"
#include "../Key.h"
#include "../Abstract.h"
#include "../Exchanger.h"

namespace librevault {

P2PProvider::P2PProvider(Session& session, Exchanger& exchanger) :
		session_(session),
		exchanger_(exchanger),
		log_(session_.log()),
		node_key_(session_),
		ssl_ctx_(boost::asio::ssl::context::tlsv12),
		acceptor_(session_.ios()) {
	// SSL settings
	ssl_ctx_.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::single_dh_use | boost::asio::ssl::context::no_sslv3);

	ssl_ctx_.use_certificate_file(session_.cert_path().string(), boost::asio::ssl::context::pem);
	ssl_ctx_.use_private_key_file(session_.key_path().string(), boost::asio::ssl::context::pem);
	ssl_ctx_.use_tmp_dh_file(session_.dhparam_path().string());
	// FIXME: Add proper encryption, finally!

	// Acceptor initialization
	url bind_url = parse_url(session_.config().get<std::string>("net.listen"));
	tcp_endpoint bound_endpoint(address::from_string(bind_url.host), bind_url.port);

	acceptor_.open(bound_endpoint.address().is_v6() ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(bound_endpoint);
	acceptor_.listen();
	log_->info() << "Listening on " << local_endpoint();

	accept_operation();

	init_persistent();
}

P2PProvider::~P2PProvider() {}

void P2PProvider::add_node(const url& node_url, std::shared_ptr<FSDirectory> directory) {
	auto connection = std::make_unique<Connection>(node_url, session_, *this);
	auto socket = std::make_shared<P2PDirectory>(std::move(connection), directory, session_, exchanger_, *this);
	unattached_connections_.insert(socket);
}

void P2PProvider::add_node(const tcp_endpoint& node_endpoint, std::shared_ptr<FSDirectory> directory) {
	auto connection = std::make_unique<Connection>(node_endpoint, session_, *this);
	auto socket = std::make_shared<P2PDirectory>(std::move(connection), directory, session_, exchanger_, *this);
	unattached_connections_.insert(socket);
}

void P2PProvider::remove_from_unattached(std::shared_ptr<P2PDirectory> already_attached) {
	unattached_connections_.erase(already_attached);
}

void P2PProvider::init_persistent() {
	auto folder_trees = session_.config().equal_range("folder");
	for(auto folder_tree_it = folder_trees.first; folder_tree_it != folder_trees.second; folder_tree_it++){
		Key k(folder_tree_it->second.get<std::string>("key"));

		auto node_tree = folder_tree_it->second.equal_range("node");
		for(auto node_tree_it = node_tree.first; node_tree_it != node_tree.second; node_tree_it++){
			url connection_url = parse_url(node_tree_it->second.get_value<std::string>());

			add_node(connection_url, exchanger_.get_directory(k.get_Hash()));
		}
	}
}

void P2PProvider::accept_operation() {
	auto socket_ptr = new ssl_socket(session_.ios(), ssl_ctx_);

	acceptor_.async_accept(socket_ptr->lowest_layer(), std::bind([this](ssl_socket* socket_ptr, const boost::system::error_code& ec) {
		if(ec == boost::asio::error::operation_aborted) return;
		log_->debug() << "TCP connection accepted from: " << socket_ptr->lowest_layer().remote_endpoint();

		auto connection = std::make_unique<Connection>(std::unique_ptr<ssl_socket>(socket_ptr), session_, *this);
		auto accepted_dir = std::make_shared<P2PDirectory>(std::move(connection), session_, exchanger_, *this);
		unattached_connections_.insert(accepted_dir);

		accept_operation();
	}, socket_ptr, std::placeholders::_1));
}

} /* namespace librevault */
