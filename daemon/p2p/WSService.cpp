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
#include "control/Paths.h"
#include "folder/FolderGroup.h"
#include "folder/FolderService.h"
#include <util/log.h>
#include <codecvt>

namespace librevault {

const char* WSService::subprotocol_ = "librevault";

WSService::WSService(io_service& ios, P2PProvider& provider, NodeKey& node_key, FolderService& folder_service) : ios_(ios), provider_(provider), node_key_(node_key), folder_service_(folder_service) {}

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
	ssl_ctx_ptr->use_certificate_file(Paths::get()->cert_path.string(std::codecvt_utf8_utf16<wchar_t>()), ssl_context::pem);
	ssl_ctx_ptr->use_private_key_file(Paths::get()->key_path.string(std::codecvt_utf8_utf16<wchar_t>()), ssl_context::pem);
	SSL_CTX_set_cipher_list(ssl_ctx_ptr->native_handle(), "ECDH-ECDSA-AES256-GCM-SHA384:ECDH-ECDSA-AES256-SHA384:ECDH-ECDSA-AES128-GCM-SHA256:ECDH-ECDSA-AES128-SHA256");

	return ssl_ctx_ptr;
}

blob WSService::pubkey_from_cert(X509* x509) {
	std::runtime_error e("Certificate error");

	std::unique_ptr<EC_GROUP, decltype(&EC_GROUP_free)> ec_group(EC_GROUP_new_by_curve_name(OBJ_sn2nid("prime256v1")), EC_GROUP_free);
	std::unique_ptr<BN_CTX, decltype(&BN_CTX_free)> bn_ctx(BN_CTX_new(), BN_CTX_free);
	std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> remote_pkey(nullptr, EVP_PKEY_free);
	std::unique_ptr<EC_KEY, decltype(&EC_KEY_free)> remote_eckey(nullptr, EC_KEY_free);

	std::vector<uint8_t> raw_public(33);

	if(ec_group == nullptr || bn_ctx == nullptr) throw e;

	remote_pkey = decltype(remote_pkey)(X509_get_pubkey(x509), EVP_PKEY_free);	if(remote_pkey == nullptr) throw e;
	remote_eckey = decltype(remote_eckey)(EVP_PKEY_get1_EC_KEY(remote_pkey.get()), EC_KEY_free);	if(remote_eckey == nullptr) throw e;
	const EC_POINT* remote_pubkey = EC_KEY_get0_public_key(remote_eckey.get());	if(remote_pubkey == nullptr) throw e;

	EC_POINT_point2oct(ec_group.get(), remote_pubkey, POINT_CONVERSION_COMPRESSED, raw_public.data(), raw_public.size(), bn_ctx.get());

	return raw_public;
}

void WSService::on_tcp_pre_init(websocketpp::connection_hdl hdl, connection::role_type role) {
	LOGFUNC();

	connection& conn = ws_assignment_[hdl];
	conn.connection_handle = hdl;
	conn.role = role;

	ws_assignment_.insert({hdl, conn});
}

std::shared_ptr<ssl_context> WSService::on_tls_init(websocketpp::connection_hdl hdl) {
	LOGFUNC();

	auto ssl_ctx_ptr = make_ssl_ctx();
	ssl_ctx_ptr->set_verify_callback(std::bind(&WSService::on_tls_verify, this, hdl, std::placeholders::_1, std::placeholders::_2));
	return ssl_ctx_ptr;
}

bool WSService::on_tls_verify(websocketpp::connection_hdl hdl, bool preverified, boost::asio::ssl::verify_context& ctx) {
	LOGFUNC();

	// FIXME: Hey, just returning `true` isn't good enough, yes?
	return true;
}

void WSService::on_open(websocketpp::connection_hdl hdl) {
	LOGFUNC();

	try {
		connection& conn = ws_assignment_[hdl];
		auto new_folder = new P2PFolder(&provider_, &node_key_, &folder_service_);
		conn.folder = new_folder;

		folder_service_.get_group(conn.hash)->attach(new_folder);

		if(conn.role == connection::CLIENT)
			new_folder->perform_handshake();
	}catch(FolderService::invalid_group& e) {
		LOGD("Folder group have been removed in process of connection: " << e.what());
		close(hdl, "Internal Server Error");
	}catch(std::exception& e) {
		LOGD("Exception occured after opening remote connection: " << e.what());
		close(hdl, "Internal Server Error");
	}
}

void WSService::on_message(websocketpp::connection_hdl hdl, const std::string& message_raw) {
	LOGFUNC();

	try {
		blob message_blob = blob(message_raw.begin(), message_raw.end());
		//std::shared_ptr<P2PFolder>(ws_assignment_[hdl].folder)->handle_message(message_blob);
	}catch(std::exception& e) {
		LOGFUNC() << " e:" << e.what();
		close(hdl, e.what());
	}
}

void WSService::on_disconnect(websocketpp::connection_hdl hdl) {
	LOGFUNC() << " e:" << errmsg(hdl);

	try {
		auto p2p_folder_ptr = ws_assignment_[hdl].folder;
		auto folder_group = p2p_folder_ptr->fgroup();

		folder_group->detach(p2p_folder_ptr);
	}catch(const std::bad_weak_ptr& e){
		LOGFUNC() << " bad pointer, what:" << e.what();
	}

	ws_assignment_.erase(hdl);
}

template<class WSClass>
void WSService::on_tcp_post_init(WSClass& c, websocketpp::connection_hdl hdl) {
	LOGFUNC();

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

		//if(provider_.is_loopback(conn.remote_pubkey) || provider_.is_loopback(conn.remote_endpoint)) {
		//	provider_.mark_loopback(conn.remote_endpoint);
		//	throw connection_error("Loopback detected");
		//}
	}catch(std::exception& e) {
		LOGFUNC() << " e:" << e.what();
		connection_ptr->terminate(websocketpp::lib::error_code());
	}
}

template void WSService::on_tcp_post_init(WSServer::server&, websocketpp::connection_hdl);
template void WSService::on_tcp_post_init(WSClient::client&, websocketpp::connection_hdl);

} /* namespace librevault */
