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
#include "WSService.h"
#include "src/Client.h"
#include "P2PFolder.h"
#include "src/folder/FolderGroup.h"

namespace librevault {

const char* WSService::subprotocol_ = "librevault";

WSService::WSService(Client& client, P2PProvider& provider) : Loggable(client, "WSService"), provider_(provider), client_(client) {}

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
	ssl_ctx_ptr->use_certificate_file(client_.cert_path().string(), ssl_context::pem);
	ssl_ctx_ptr->use_private_key_file(client_.key_path().string(), ssl_context::pem);
	SSL_CTX_set_cipher_list(ssl_ctx_ptr->native_handle(), "ECDH+ECDSA+AES:!SHA");

	return ssl_ctx_ptr;
}

blob WSService::pubkey_from_cert(X509* x509) {
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

std::shared_ptr<ssl_context> WSService::on_tls_init(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << "on_tls_init()";

	auto ssl_ctx_ptr = make_ssl_ctx();
	ssl_ctx_ptr->set_verify_callback(std::bind(&WSService::on_tls_verify, this, hdl, std::placeholders::_1, std::placeholders::_2));
	return ssl_ctx_ptr;
}

bool WSService::on_tls_verify(websocketpp::connection_hdl hdl, bool preverified, boost::asio::ssl::verify_context& ctx) {
	log_->trace() << log_tag() << "on_tls_verify()";

	// FIXME: Hey, just returning `true` isn't good enough, yes?
	return true;
}

void WSService::on_open(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << "on_open()";

	auto dir_ptr = dir_ptr_from_hdl(hdl);
	log_->debug() << log_tag() << "Connection opened to: " << dir_ptr->name();
	if(dir_ptr->role() == P2PProvider::CLIENT)
		dir_ptr->perform_handshake();
}

void WSService::on_message(websocketpp::connection_hdl hdl, const std::string& message_raw) {
	log_->trace() << log_tag() << "on_message()";

	try {
		blob message_blob = blob(std::make_move_iterator(message_raw.begin()), std::make_move_iterator(message_raw.end()));
		dir_ptr_from_hdl(hdl)->handle_message(message_blob);
	}catch(std::runtime_error& e) {
		log_->trace() << log_tag() << "on_message e:" << e.what();
		close(hdl, e.what());
	}
}

void WSService::on_disconnect(websocketpp::connection_hdl hdl) {
	log_->trace() << log_tag() << "on_disconnect()";

	auto dir_ptr = dir_ptr_from_hdl(hdl);
	if(dir_ptr) {
		auto folder_group = dir_ptr->folder_group();
		if(folder_group)
			folder_group->detach(dir_ptr);
	}
	ws_assignment_.erase(hdl);
}

std::shared_ptr<P2PFolder> WSService::dir_ptr_from_hdl(websocketpp::connection_hdl hdl) {
	auto it_server = ws_assignment_.find(hdl);
	return it_server != ws_assignment_.end() ? it_server->second : nullptr;
}

} /* namespace librevault */
