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
#include "P2PProvider.h"
#include "Version.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "folder/FolderService.h"
#include "nodekey/NodeKey.h"
#include "util/readable.h"
#include "util/conv_bitarray.h"
#include <librevault/Tokens.h>
#include <librevault/protocol/V1Parser.h>

namespace librevault {

P2PFolder::P2PFolder(FolderGroup* fgroup, NodeKey* node_key, QObject* parent) :
	QObject(parent),
	node_key_(node_key),
	fgroup_(fgroup) {
	LOGFUNC();

	resetUnderlyingSocket(new QWebSocket(Version().user_agent()));

	// Internal signal interconnection
	connect(this, &P2PFolder::handshakeFailed, this, &P2PFolder::handleDisconnected);
}

P2PFolder::~P2PFolder() {
	LOGFUNC();
	fgroup_->detach(this);
}

void P2PFolder::resetUnderlyingSocket(QWebSocket* socket) {
	if(socket_) socket_->deleteLater();
	socket_ = socket;
	socket_->setParent(this);

	connect(socket_, &QWebSocket::pong, this, &P2PFolder::handlePong);
	connect(socket_, &QWebSocket::binaryMessageReceived, this, &P2PFolder::handleMessage);
	connect(socket_, &QWebSocket::connected, this, &P2PFolder::handleConnected);
	connect(socket_, &QWebSocket::aboutToClose, this, [=]{fgroup_->detach(this);});
	connect(socket_, &QWebSocket::disconnected, this, &P2PFolder::handleDisconnected);
}

void P2PFolder::setConnectedSocket(QWebSocket* socket) {
	resetUnderlyingSocket(socket);

	role_ = Role::SERVER;

	startTimeout();
	handleConnected();
}

void P2PFolder::open(QUrl url) {
	resetUnderlyingSocket(new QWebSocket(Version().user_agent()));

	role_ = Role::CLIENT;

	startTimeout();
	socket_->setSslConfiguration(P2PProvider::getSslConfiguration(node_key_));
	socket_->open(url);
}

void P2PFolder::startPinger() {
	if(ping_timer_) ping_timer_->deleteLater();
	ping_timer_ = new QTimer();

	ping_timer_->setInterval(20*1000);
	ping_timer_->start();

	connect(ping_timer_, &QTimer::timeout, this, [this]{socket_->ping();});
}

void P2PFolder::startTimeout() {
	if(timeout_timer_) timeout_timer_->deleteLater();
	timeout_timer_ = new QTimer();

	connect(timeout_timer_, &QTimer::timeout, this, &P2PFolder::handleDisconnected);

	timeout_timer_->setSingleShot(true);
	bumpTimeout();
	timeout_timer_->start();
}

QString P2PFolder::displayName() const {
	switch(socket_->peerAddress().protocol()) {
		case QAbstractSocket::IPv4Protocol:
			return QString("%1:%2").arg(socket_->peerAddress().toString()).arg(socket_->peerPort());
		case QAbstractSocket::IPv6Protocol:
			return QString("[%1]:%2").arg(socket_->peerAddress().toString()).arg(socket_->peerPort());
		default:
			return "";
	}
}

QByteArray P2PFolder::digest() const {
	return socket_->sslConfiguration().peerCertificate().digest(node_key_->digestAlgorithm());
}

QPair<QHostAddress, quint16> P2PFolder::endpoint() const {
	return {socket_->peerAddress(), socket_->peerPort()};
}

QJsonObject P2PFolder::collect_state() {
	QJsonObject state;

	state["endpoint"] = displayName();   //FIXME: Must be host:port
	state["client_name"] = client_name();
	state["user_agent"] = user_agent();
	state["traffic_stats"] = counter_.heartbeat_json();
	state["rtt"] = double(rtt_.count());

	return state;
}

blob P2PFolder::derive_token_digest(const Secret& secret, QByteArray digest) {
	return derive_token(secret, conv_bytearray(digest));
}

blob P2PFolder::local_token() {
	return derive_token_digest(fgroup_->params().secret, node_key_->digest());
}

blob P2PFolder::remote_token() {
	return derive_token_digest(fgroup_->params().secret, digest());
}



/* InterestGuard */
P2PFolder::InterestGuard::InterestGuard(P2PFolder* remote) : remote_(remote) {
	remote_->sendInterested();
}

P2PFolder::InterestGuard::~InterestGuard() {
	remote_->sendNotInterested();
}

std::shared_ptr<P2PFolder::InterestGuard> P2PFolder::get_interest_guard() {
	try {
		return std::shared_ptr<InterestGuard>(interest_guard_);
	}catch(std::bad_weak_ptr& e){
		auto guard = std::make_shared<InterestGuard>(this);
		interest_guard_ = guard;
		return guard;
	}
}

/* RPC Actions */
void P2PFolder::sendMessage(const QByteArray& message) {
	counter_.add_up(message.size());
	socket_->sendBinaryMessage(message);
}

void P2PFolder::sendMessage(const blob& message) {
	sendMessage(QByteArray::fromRawData((char*)message.data(), message.size()));
}

void P2PFolder::sendHandshake() {
	V1Parser::Handshake message_struct;
	message_struct.auth_token = local_token();
	message_struct.device_name = Config::get()->getGlobal("client_name").toString().toStdString();
	message_struct.user_agent = Version::current().user_agent().toStdString();

	sendMessage(V1Parser().gen_Handshake(message_struct));
	handshake_sent_ = true;
	LOGD("==> HANDSHAKE");
}

void P2PFolder::sendChoke() {
	if(! am_choking_) {
		sendMessage(V1Parser().gen_Choke());
		am_choking_ = true;

		LOGD("==> CHOKE");
	}
}
void P2PFolder::sendUnchoke() {
	if(am_choking_) {
		sendMessage(V1Parser().gen_Unchoke());
		am_choking_ = false;

		LOGD("==> UNCHOKE");
	}
}
void P2PFolder::sendInterested() {
	if(! am_interested_) {
		sendMessage(V1Parser().gen_Interested());
		am_interested_ = true;

		LOGD("==> INTERESTED");
	}
}
void P2PFolder::sendNotInterested() {
	if(am_interested_) {
		sendMessage(V1Parser().gen_NotInterested());
		am_interested_ = false;

		LOGD("==> NOT_INTERESTED");
	}
}

void P2PFolder::sendHaveMeta(const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	V1Parser::HaveMeta message;
	message.revision = revision;
	message.bitfield = bitfield;
	sendMessage(V1Parser().gen_HaveMeta(message));

	LOGD("==> HAVE_META:"
		<< " path_id=" << path_id_readable(message.revision.path_id_)
		<< " revision=" << message.revision.revision_
		<< " bits=" << conv_bitarray(message.bitfield));
}
void P2PFolder::sendHaveChunk(const blob& ct_hash) {
	V1Parser::HaveChunk message;
	message.ct_hash = ct_hash;
	sendMessage(V1Parser().gen_HaveChunk(message));

	LOGD("==> HAVE_BLOCK:"
		<< " ct_hash=" << ct_hash_readable(ct_hash));
}

void P2PFolder::sendMetaRequest(const Meta::PathRevision& revision) {
	V1Parser::MetaRequest message;
	message.revision = revision;
	sendMessage(V1Parser().gen_MetaRequest(message));

	LOGD("==> META_REQUEST:"
		<< " path_id=" << path_id_readable(revision.path_id_)
		<< " revision=" << revision.revision_);
}
void P2PFolder::sendMetaReply(const SignedMeta& smeta, const bitfield_type& bitfield) {
	V1Parser::MetaReply message;
	message.smeta = smeta;
	message.bitfield = bitfield;
	sendMessage(V1Parser().gen_MetaReply(message));

	LOGD("==> META_REPLY:"
		<< " path_id=" << path_id_readable(smeta.meta().path_id())
		<< " revision=" << smeta.meta().revision()
		<< " bits=" << conv_bitarray(bitfield));
}
void P2PFolder::sendMetaCancel(const Meta::PathRevision& revision) {
	V1Parser::MetaCancel message;
	message.revision = revision;
	sendMessage(V1Parser().gen_MetaCancel(message));

	LOGD("==> META_CANCEL:"
		<< " path_id=" << path_id_readable(revision.path_id_)
		<< " revision=" << revision.revision_);
}

void P2PFolder::sendBlockRequest(const blob& ct_hash, uint32_t offset, uint32_t length) {
	V1Parser::BlockRequest message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.length = length;
	sendMessage(V1Parser().gen_BlockRequest(message));

	LOGD("==> BLOCK_REQUEST:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset
		<< " length=" << length);
}
void P2PFolder::sendBlockReply(const blob& ct_hash, uint32_t offset, const blob& block) {
	V1Parser::BlockReply message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.content = block;
	sendMessage(V1Parser().gen_BlockReply(message));

	counter_.add_up_blocks(block.size());

	LOGD("==> BLOCK_REPLY:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset);
}
void P2PFolder::sendBlockCancel(const blob& ct_hash, uint32_t offset, uint32_t length) {
	V1Parser::BlockCancel message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.length = length;
	sendMessage(V1Parser().gen_BlockCancel(message));
	LOGD("==> BLOCK_CANCEL:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset
		<< " length=" << length);
}

void P2PFolder::handleMessage(const QByteArray& message) {
	blob message_raw(message.begin(), message.end());
	V1Parser::message_type message_type = V1Parser().parse_MessageType(message_raw);

	counter_.add_down(message_raw.size());

	bumpTimeout();

	if(ready()) {
		switch(message_type) {
			case V1Parser::CHOKE: handleChoke(message_raw); break;
			case V1Parser::UNCHOKE: handleUnchoke(message_raw); break;
			case V1Parser::INTERESTED: handleInterested(message_raw); break;
			case V1Parser::NOT_INTERESTED: handleNotInterested(message_raw); break;
			case V1Parser::HAVE_META: handleHaveMeta(message_raw); break;
			case V1Parser::HAVE_CHUNK: handleHaveChunk(message_raw); break;
			case V1Parser::META_REQUEST: handleMetaRequest(message_raw); break;
			case V1Parser::META_REPLY: handleMetaReply(message_raw); break;
			case V1Parser::META_CANCEL: handleMetaCancel(message_raw); break;
			case V1Parser::BLOCK_REQUEST: handleBlockRequest(message_raw); break;
			case V1Parser::BLOCK_REPLY: handleBlockReply(message_raw); break;
			case V1Parser::BLOCK_CANCEL: handleBlockCancel(message_raw); break;
			default: socket_->close(QWebSocketProtocol::CloseCodeProtocolError);
		}
	}else{
		handleHandshake(message_raw);
	}
}

void P2PFolder::handleHandshake(const blob& message_raw) {
	LOGFUNC();
	try {
		auto message_struct = V1Parser().parse_Handshake(message_raw);
		LOGD("<== HANDSHAKE");

		// Checking authentication using token
		if(message_struct.auth_token != remote_token()) throw auth_error();

		if(role_ == SERVER) sendHandshake();

		client_name_ = QString::fromStdString(message_struct.device_name);
		user_agent_ = QString::fromStdString(message_struct.user_agent);

		LOGD("LV Handshake successful");
		handshake_received_ = true;

		emit handshakeSuccess();
	}catch(std::exception& e){
		emit handshakeFailed();
	}
}

void P2PFolder::handleChoke(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== CHOKE");

	if(! peer_choking_) {
		peer_choking_ = true;
		emit rcvdChoke();
	}
}
void P2PFolder::handleUnchoke(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== UNCHOKE");

	if(peer_choking_) {
		peer_choking_ = false;
		emit rcvdUnchoke();
	}
}
void P2PFolder::handleInterested(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== INTERESTED");

	if(! peer_interested_) {
		peer_interested_ = true;
		emit rcvdInterested();
	}
}
void P2PFolder::handleNotInterested(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== NOT_INTERESTED");

	if(peer_interested_) {
		peer_interested_ = false;
		emit rcvdNotInterested();
	}
}

void P2PFolder::handleHaveMeta(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_HaveMeta(message_raw);
	LOGD("<== HAVE_META:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_
		<< " bits=" << conv_bitarray(message_struct.bitfield));

	emit rcvdHaveMeta(message_struct.revision, message_struct.bitfield);
}
void P2PFolder::handleHaveChunk(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_HaveChunk(message_raw);
	LOGD("<== HAVE_BLOCK:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash));
	emit rcvdHaveChunk(message_struct.ct_hash);
}

void P2PFolder::handleMetaRequest(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_MetaRequest(message_raw);
	LOGD("<== META_REQUEST:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_);

	emit rcvdMetaRequest(message_struct.revision);
}
void P2PFolder::handleMetaReply(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_MetaReply(message_raw, fgroup_->params().secret);
	LOGD("<== META_REPLY:"
		<< " path_id=" << path_id_readable(message_struct.smeta.meta().path_id())
		<< " revision=" << message_struct.smeta.meta().revision()
		<< " bits=" << conv_bitarray(message_struct.bitfield));

	emit rcvdMetaReply(message_struct.smeta, message_struct.bitfield);
}
void P2PFolder::handleMetaCancel(const blob& message_raw) {
#   warning "Not implemented yet"
	LOGFUNC();

	auto message_struct = V1Parser().parse_MetaCancel(message_raw);
	LOGD("<== META_CANCEL:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_);

	emit rcvdMetaCancel(message_struct.revision);
}

void P2PFolder::handleBlockRequest(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_BlockRequest(message_raw);
	LOGD("<== BLOCK_REQUEST:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " length=" << message_struct.length
		<< " offset=" << message_struct.offset);

	emit rcvdBlockRequest(message_struct.ct_hash, message_struct.offset, message_struct.length);
}
void P2PFolder::handleBlockReply(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = V1Parser().parse_BlockReply(message_raw);
	LOGD("<== BLOCK_REPLY:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " offset=" << message_struct.offset);

	counter_.add_down_blocks(message_struct.content.size());

	emit rcvdBlockReply(message_struct.ct_hash, message_struct.offset, message_struct.content);
}
void P2PFolder::handleBlockCancel(const blob& message_raw) {
#   warning "Not implemented yet"
	LOGFUNC();

	auto message_struct = V1Parser().parse_BlockCancel(message_raw);
	LOGD("<== BLOCK_CANCEL:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " length=" << message_struct.length
		<< " offset=" << message_struct.offset);

	emit rcvdBlockCancel(message_struct.ct_hash, message_struct.offset, message_struct.length);
}

void P2PFolder::bumpTimeout() {
	timeout_timer_->setInterval(120*1000);
}

void P2PFolder::handlePong(quint64 rtt) {
	bumpTimeout();
	rtt_ = std::chrono::milliseconds(rtt);
}

void P2PFolder::handleConnected() {
	startPinger();

	if(fgroup_->attach(this)) {
		if(role_ == CLIENT) sendHandshake();
	}else
		socket_->close(QWebSocketProtocol::CloseCodePolicyViolated);
}

} /* namespace librevault */
