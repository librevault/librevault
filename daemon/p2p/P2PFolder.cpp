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
#include "P2PFolder.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "folder/FolderService.h"
#include "nodekey/NodeKey.h"
#include "util/log.h"
#include "util/readable.h"
#include <librevault/Tokens.h>
#include <librevault/protocol/V1Parser.h>

namespace librevault {

P2PFolder::P2PFolder(QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key, Role role) :
	RemoteFolder(fgroup),
	role_(role),
	provider_(provider),
	node_key_(node_key),
	socket_(socket),
	fgroup_(fgroup) {
	LOGFUNC();

	socket->setParent(this);
	this->setParent(fgroup);

	// Set up timers
	ping_timer_ = new QTimer(this);
	timeout_timer_ = new QTimer(this);

	// Connect signals
	connect(ping_timer_, &QTimer::timeout, socket_, [this]{socket_->ping();});
	connect(timeout_timer_, &QTimer::timeout, socket_, [this]{socket_->close(QWebSocketProtocol::CloseCodeAbnormalDisconnection);});
	connect(socket_, &QWebSocket::pong, this, &P2PFolder::handlePong);
	connect(socket_, &QWebSocket::binaryMessageReceived, this, &P2PFolder::handle_message);
	connect(socket_, &QWebSocket::stateChanged, this, &P2PFolder::handleWebSocketStateChanged);

	// Start timers
	ping_timer_->setInterval(20*1000);
	ping_timer_->start();

	timeout_timer_->setSingleShot(true);
	bump_timeout();
	timeout_timer_->start();
}

P2PFolder::P2PFolder(QUrl url, QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key) :
	P2PFolder(socket, fgroup, provider, node_key, CLIENT) {

	socket_->setSslConfiguration(provider_->getSslConfiguration());
	socket_->open(url);
}

P2PFolder::P2PFolder(QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key) :
	P2PFolder(socket, fgroup, provider, node_key, SERVER) {

}

P2PFolder::~P2PFolder() {
	LOGFUNC();
}

QString P2PFolder::displayName() const {
	std::ostringstream os; os << remote_endpoint();
	return QString::fromStdString(os.str());
}

QByteArray P2PFolder::digest() const {
	return socket_->sslConfiguration().peerCertificate().digest(node_key_->digestAlgorithm());
}

tcp_endpoint P2PFolder::remote_endpoint() const {
	try {
		return tcp_endpoint(address::from_string(socket_->peerAddress().toString().toStdString()), socket_->peerPort());
	}catch(std::exception& e){
		return tcp_endpoint();
	}
}

Json::Value P2PFolder::collect_state() {
	Json::Value state;

	std::ostringstream os; os << remote_endpoint();
	state["endpoint"] = os.str();
	state["client_name"] = client_name();
	state["user_agent"] = user_agent();
	state["traffic_stats"] = counter_.heartbeat_json();
	state["rtt"] = Json::Value::UInt64(rtt_.count());

	return state;
}

blob P2PFolder::derive_token_digest(const Secret& secret, QByteArray digest) {
	return derive_token(secret, blob(digest.begin(), digest.end()));
}

blob P2PFolder::local_token() {
	return derive_token_digest(fgroup_->secret(), node_key_->digest());
}

blob P2PFolder::remote_token() {
	return derive_token_digest(fgroup_->secret(), digest());
}

void P2PFolder::send_message(const blob& message) {
	counter_.add_up(message.size());
	fgroup_->bandwidth_counter().add_up(message.size());
	socket_->sendBinaryMessage(QByteArray::fromRawData((char*)message.data(), message.size()));
}

void P2PFolder::sendHandshake() {
	V1Parser::Handshake message_struct;
	message_struct.auth_token = local_token();
	message_struct.device_name = Config::get()->global_get("client_name").asString();
	message_struct.user_agent = Version::current().user_agent().toStdString();

	send_message(V1Parser().gen_Handshake(message_struct));
	handshake_sent_ = true;
	LOGD("==> HANDSHAKE");
}

/* RPC Actions */
void P2PFolder::choke() {
	if(! am_choking_) {
		send_message(V1Parser().gen_Choke());
		am_choking_ = true;

		LOGD("==> CHOKE");
	}
}
void P2PFolder::unchoke() {
	if(am_choking_) {
		send_message(V1Parser().gen_Unchoke());
		am_choking_ = false;

		LOGD("==> UNCHOKE");
	}
}
void P2PFolder::interest() {
	if(! am_interested_) {
		send_message(V1Parser().gen_Interested());
		am_interested_ = true;

		LOGD("==> INTERESTED");
	}
}
void P2PFolder::uninterest() {
	if(am_interested_) {
		send_message(V1Parser().gen_NotInterested());
		am_interested_ = false;

		LOGD("==> NOT_INTERESTED");
	}
}

void P2PFolder::post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	V1Parser::HaveMeta message;
	message.revision = revision;
	message.bitfield = bitfield;
	send_message(V1Parser().gen_HaveMeta(message));

	LOGD("==> HAVE_META:"
		<< " path_id=" << path_id_readable(message.revision.path_id_)
		<< " revision=" << message.revision.revision_
		<< " bits=" << message.bitfield);
}
void P2PFolder::post_have_chunk(const blob& ct_hash) {
	V1Parser::HaveChunk message;
	message.ct_hash = ct_hash;
	send_message(V1Parser().gen_HaveChunk(message));

	LOGD("==> HAVE_BLOCK:"
		<< " ct_hash=" << ct_hash_readable(ct_hash));
}

void P2PFolder::request_meta(const Meta::PathRevision& revision) {
	V1Parser::MetaRequest message;
	message.revision = revision;
	send_message(V1Parser().gen_MetaRequest(message));

	LOGD("==> META_REQUEST:"
		<< " path_id=" << path_id_readable(revision.path_id_)
		<< " revision=" << revision.revision_);
}
void P2PFolder::post_meta(const SignedMeta& smeta, const bitfield_type& bitfield) {
	V1Parser::MetaReply message;
	message.smeta = smeta;
	message.bitfield = bitfield;
	send_message(V1Parser().gen_MetaReply(message));

	LOGD("==> META_REPLY:"
		<< " path_id=" << path_id_readable(smeta.meta().path_id())
		<< " revision=" << smeta.meta().revision()
		<< " bits=" << bitfield);
}
void P2PFolder::cancel_meta(const Meta::PathRevision& revision) {
	V1Parser::MetaCancel message;
	message.revision = revision;
	send_message(V1Parser().gen_MetaCancel(message));

	LOGD("==> META_CANCEL:"
		<< " path_id=" << path_id_readable(revision.path_id_)
		<< " revision=" << revision.revision_);
}

void P2PFolder::request_block(const blob& ct_hash, uint32_t offset, uint32_t length) {
	V1Parser::BlockRequest message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.length = length;
	send_message(V1Parser().gen_BlockRequest(message));

	LOGD("==> BLOCK_REQUEST:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset
		<< " length=" << length);
}
void P2PFolder::post_block(const blob& ct_hash, uint32_t offset, const blob& block) {
	V1Parser::BlockReply message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.content = block;
	send_message(V1Parser().gen_BlockReply(message));

	counter_.add_up_blocks(block.size());
	fgroup_->bandwidth_counter().add_up_blocks(block.size());

	LOGD("==> BLOCK_REPLY:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset);
}
void P2PFolder::cancel_block(const blob& ct_hash, uint32_t offset, uint32_t length) {
	V1Parser::BlockCancel message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.length = length;
	send_message(V1Parser().gen_BlockCancel(message));
	LOGD("==> BLOCK_CANCEL:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset
		<< " length=" << length);
}

void P2PFolder::handle_message(const QByteArray& message) {
	blob message_raw(message.begin(), message.end());
	V1Parser::message_type message_type = V1Parser().parse_MessageType(message_raw);

	counter_.add_down(message_raw.size());
	fgroup_->bandwidth_counter().add_down(message_raw.size());

	bump_timeout();

	if(ready()) {
		switch(message_type) {
			case V1Parser::CHOKE: handle_Choke(message_raw); break;
			case V1Parser::UNCHOKE: handle_Unchoke(message_raw); break;
			case V1Parser::INTERESTED: handle_Interested(message_raw); break;
			case V1Parser::NOT_INTERESTED: handle_NotInterested(message_raw); break;
			case V1Parser::HAVE_META: handle_HaveMeta(message_raw); break;
			case V1Parser::HAVE_CHUNK: handle_HaveChunk(message_raw); break;
			case V1Parser::META_REQUEST: handle_MetaRequest(message_raw); break;
			case V1Parser::META_REPLY: handle_MetaReply(message_raw); break;
			case V1Parser::META_CANCEL: handle_MetaCancel(message_raw); break;
			case V1Parser::BLOCK_REQUEST: handle_BlockRequest(message_raw); break;
			case V1Parser::BLOCK_REPLY: handle_BlockReply(message_raw); break;
			case V1Parser::BLOCK_CANCEL: handle_BlockCancel(message_raw); break;
			default: throw protocol_error();
		}
	}else{
		handle_Handshake(message_raw);
	}
}

void P2PFolder::handle_Handshake(const blob& message_raw) {
	LOGFUNC();
	auto message_struct = V1Parser().parse_Handshake(message_raw);
	LOGD("<== HANDSHAKE");

	// Checking authentication using token
	if(message_struct.auth_token != remote_token()) throw auth_error();

	if(role_ == SERVER) sendHandshake();

	client_name_ = message_struct.device_name;
	user_agent_ = message_struct.user_agent;

	LOGD("LV Handshake successful");
	handshake_received_ = true;

	emit handshakePerformed();
}

void P2PFolder::handle_Choke(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== CHOKE");

	if(! peer_choking_) {
		peer_choking_ = true;
		emit rcvdChoke();
	}
}
void P2PFolder::handle_Unchoke(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== UNCHOKE");

	if(peer_choking_) {
		peer_choking_ = false;
		emit rcvdUnchoke();
	}
}
void P2PFolder::handle_Interested(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== INTERESTED");

	if(! peer_interested_) {
		peer_interested_ = true;
		emit rcvdInterested();
	}
}
void P2PFolder::handle_NotInterested(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== NOT_INTERESTED");

	if(peer_interested_) {
		peer_interested_ = false;
		emit rcvdNotInterested();
	}
}

void P2PFolder::handle_HaveMeta(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_HaveMeta(message_raw);
	LOGD("<== HAVE_META:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_
		<< " bits=" << message_struct.bitfield);

	emit rcvdHaveMeta(message_struct.revision, message_struct.bitfield);
}
void P2PFolder::handle_HaveChunk(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_HaveChunk(message_raw);
	LOGD("<== HAVE_BLOCK:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash));
	emit rcvdHaveChunk(message_struct.ct_hash);
}

void P2PFolder::handle_MetaRequest(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_MetaRequest(message_raw);
	LOGD("<== META_REQUEST:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_);

	emit rcvdMetaRequest(message_struct.revision);
}
void P2PFolder::handle_MetaReply(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_MetaReply(message_raw, fgroup_->secret());
	LOGD("<== META_REPLY:"
		<< " path_id=" << path_id_readable(message_struct.smeta.meta().path_id())
		<< " revision=" << message_struct.smeta.meta().revision()
		<< " bits=" << message_struct.bitfield);

	emit rcvdMetaReply(message_struct.smeta, message_struct.bitfield);
}
void P2PFolder::handle_MetaCancel(const blob& message_raw) {
#   warning "Not implemented yet"
	LOGFUNC();

	auto message_struct = V1Parser().parse_MetaCancel(message_raw);
	LOGD("<== META_CANCEL:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_);

	emit rcvdMetaCancel(message_struct.revision);
}

void P2PFolder::handle_BlockRequest(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_BlockRequest(message_raw);
	LOGD("<== BLOCK_REQUEST:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " length=" << message_struct.length
		<< " offset=" << message_struct.offset);

	emit rcvdBlockRequest(message_struct.ct_hash, message_struct.offset, message_struct.length);
}
void P2PFolder::handle_BlockReply(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_BlockReply(message_raw);
	LOGD("<== BLOCK_REPLY:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " offset=" << message_struct.offset);

	counter_.add_down_blocks(message_struct.content.size());
	fgroup_->bandwidth_counter().add_down_blocks(message_struct.content.size());

	emit rcvdBlockReply(message_struct.ct_hash, message_struct.offset, message_struct.content);
}
void P2PFolder::handle_BlockCancel(const blob& message_raw) {
#   warning "Not implemented yet"
	LOGFUNC();

	auto message_struct = V1Parser().parse_BlockCancel(message_raw);
	LOGD("<== BLOCK_CANCEL:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " length=" << message_struct.length
		<< " offset=" << message_struct.offset);

	emit rcvdBlockCancel(message_struct.ct_hash, message_struct.offset, message_struct.length);
}

void P2PFolder::bump_timeout() {
	timeout_timer_->setInterval(120*1000);
}

void P2PFolder::handlePong(quint64 rtt) {
	bump_timeout();
	rtt_ = std::chrono::milliseconds(rtt);
}

void P2PFolder::handleWebSocketStateChanged(QAbstractSocket::SocketState state) {
	LOGT("State changed: " << state);
	if(state == QAbstractSocket::ConnectedState && role_ == CLIENT) {
		sendHandshake();
	}
	if(state == QAbstractSocket::UnconnectedState) {
		deleteLater();
	}
}

} /* namespace librevault */
