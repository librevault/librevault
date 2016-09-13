/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "WSService.h"
#include "WSServer.h"
#include "WSClient.h"
#include "P2PFolder.h"
#include "Client.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include <codecvt>

namespace librevault {

const char* WSService::subprotocol_ = "librevault";

WSService::WSService(Client& client, P2PProvider& provider) : Loggable("WSService"), provider_(provider), client_(client) {}

std::shared_ptr<ssl_context> WSService::make_ssl_ctx() {
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
	ssl_ctx_ptr->use_certificate_file(Config::get()->paths().cert_path.string(std::codecvt_utf8_utf16<wchar_t>()), ssl_context::pem);
	ssl_ctx_ptr->use_private_key_file(Config::get()->paths().key_path.string(std::codecvt_utf8_utf16<wchar_t>()), ssl_context::pem);
	SSL_CTX_set_cipher_list(ssl_ctx_ptr->native_handle(), "ECDH-ECDSA-AES256-GCM-SHA384:ECDH-ECDSA-AES256-SHA384:ECDH-ECDSA-AES128-GCM-SHA256:ECDH-ECDSA-AES128-SHA256");

	return ssl_ctx_ptr;
}

blob WSService::pubkey_from_cert(X509* x509) {
	std::runtime_error e("Certificate error");

	EC_GROUP* ec_group = EC_GROUP_new_by_curve_name(OBJ_sn2nid("prime256v1"));
	BN_CTX* bn_ctx = BN_CTX_new();
	EVP_PKEY* remote_pkey = nullptr;
	EC_KEY* remote_eckey = nullptr;

	std::vector<uint8_t> raw_public(33);

	try {
		if(ec_group == nullptr || bn_ctx == nullptr) throw e;

		remote_pkey = X509_get_pubkey(x509);	if(remote_pkey == nullptr) throw e;
		remote_eckey = EVP_PKEY_get1_EC_KEY(remote_pkey);	if(remote_eckey == nullptr) throw e;
		const EC_POINT* remote_pubkey = EC_KEY_get0_public_key(remote_eckey);	if(remote_pubkey == nullptr) throw e;

		EC_POINT_point2oct(ec_group, remote_pubkey, POINT_CONVERSION_COMPRESSED, raw_public.data(), raw_public.size(), bn_ctx);
	}catch(...){
		EC_GROUP_free(ec_group); BN_CTX_free(bn_ctx); EVP_PKEY_free(remote_pkey); EC_KEY_free(remote_eckey);
		ec_group = nullptr; bn_ctx = nullptr; remote_pkey = nullptr; remote_eckey = nullptr;

		throw;
	}

	EC_GROUP_free(ec_group); BN_CTX_free(bn_ctx); EVP_PKEY_free(remote_pkey); EC_KEY_free(remote_eckey);

	return raw_public;
}

void WSService::on_tcp_pre_init(websocketpp::connection_hdl hdl, connection::role_type role) {
	log_->trace() << log_tag() << "on_tcp_pre_init()";

	connection& conn = ws_assignment_[hdl];
	conn.connection_handle = hdl;
	conn.role = role;

	ws_assignment_.insert({hdl, conn});
}

std::shared_ptr<ssl_context> WSService::on_tls_init(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto ssl_ctx_ptr = make_ssl_ctx();
	ssl_ctx_ptr->set_verify_callback(std::bind(&WSService::on_tls_verify, this, hdl, std::placeholders::_1, std::placeholders::_2));
	return ssl_ctx_ptr;
}

bool WSService::on_tls_verify(websocketpp::connection_hdl hdl, bool preverified, boost::asio::ssl::verify_context& ctx) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	// FIXME: Hey, just returning `true` isn't good enough, yes?
	return true;
}

void WSService::on_open(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	connection& conn = ws_assignment_[hdl];
	auto new_folder = std::make_shared<P2PFolder>(client_, provider_, *this, conn);
	conn.folder = new_folder;

	auto group_ptr = client_.get_group(conn.hash);
	if(group_ptr) {
		log_->debug() << log_tag() << "Connection opened to: " << new_folder->name();   // Finally!

		try {
			group_ptr->attach(new_folder);
		}catch(std::exception& e){
			close(hdl, "Couldn't attach to remote node");
		}

		if(conn.role == connection::CLIENT)
			new_folder->perform_handshake();
	}else{
		close(hdl, "Internal Server Error");    // Apparently, the group had been deleted before we opened this connection. This is very unlikely.
	}
}

void WSService::on_message(websocketpp::connection_hdl hdl, const std::string& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	try {
		blob message_blob = blob(message_raw.begin(), message_raw.end());
		std::shared_ptr<P2PFolder>(ws_assignment_[hdl].folder)->handle_message(message_blob);
	}catch(std::exception& e) {
		log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION << " e:" << e.what();
		close(hdl, e.what());
	}
}

void WSService::on_disconnect(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION << " e:" << errmsg(hdl);

	auto dir_ptr = ws_assignment_[hdl].folder.lock();
	if(dir_ptr) {
		try {
			auto folder_group = dir_ptr->folder_group();
			if(folder_group)
				folder_group->detach(dir_ptr);
		}catch(const std::bad_weak_ptr& e){
			log_->debug() << log_tag() << BOOST_CURRENT_FUNCTION << " e:" << e.what();
		}
	}
	ws_assignment_.erase(hdl);
}

template<class WSClass>
void WSService::on_tcp_post_init(WSClass& c, websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto connection_ptr = c.get_con_from_hdl(hdl);
	if(!c.is_server())
		connection_ptr->add_subprotocol(subprotocol_);

	try {
		// Validate SSL certificate
		X509* x509 = SSL_get_peer_certificate(connection_ptr->get_socket().native_handle());
		if(!x509) throw connection_error("Certificate error");

		// Detect loopback
		connection& conn = ws_assignment_[hdl];
		conn.remote_pubkey = pubkey_from_cert(x509);
		conn.remote_endpoint = connection_ptr->get_raw_socket().remote_endpoint();

		X509_free(x509);

		if(provider_.is_loopback(conn.remote_pubkey) || provider_.is_loopback(conn.remote_endpoint)) {
			provider_.mark_loopback(conn.remote_endpoint);
			throw connection_error("Loopback detected");
		}
	}catch(std::exception& e) {
		log_->warn() << log_tag() << BOOST_CURRENT_FUNCTION << " e:" << e.what();
		connection_ptr->terminate(websocketpp::lib::error_code());
	}
}

template void WSService::on_tcp_post_init(WSServer::server&, websocketpp::connection_hdl);
template void WSService::on_tcp_post_init(WSClient::client&, websocketpp::connection_hdl);

} /* namespace librevault */
