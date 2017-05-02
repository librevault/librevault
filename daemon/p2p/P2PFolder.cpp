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
#include "HandshakeHandler.h"
#include "PingHandler.h"
#include "Version.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "nodekey/NodeKey.h"
#include "util/readable.h"
#include "util/conv_bitarray.h"
#include <librevault/protocol/V1Parser.h>

namespace librevault {

P2PFolder::P2PFolder(FolderGroup* fgroup, NodeKey* node_key, QObject* parent) :
	QObject(parent),
	node_key_(node_key),
	fgroup_(fgroup) {
	LOGFUNC();

	resetUnderlyingSocket(new QWebSocket(Version().user_agent()));

	handshake_handler_ = new HandshakeHandler(fgroup_->params(), Config::get()->getGlobal("client_name").toString(), Version().user_agent(), {}, this);
	connect(handshake_handler_, &HandshakeHandler::handshakeSuccess, this, &P2PFolder::handshakeSuccess);
	connect(handshake_handler_, &HandshakeHandler::handshakeFailed, this, &P2PFolder::handshakeFailed);
	connect(handshake_handler_, &HandshakeHandler::messagePrepared, this, qOverload<QByteArray>(&P2PFolder::sendMessage));

	ping_handler_ = new PingHandler();
	timeout_handler_ = new TimeoutHandler();
	connect(timeout_handler_, &TimeoutHandler::timedOut, this, &P2PFolder::handleDisconnected);

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

	connect(ping_handler_, &PingHandler::sendPing, socket_, &QWebSocket::ping);
	connect(socket_, &QWebSocket::pong, ping_handler_, &PingHandler::handlePong);
	connect(socket_, &QWebSocket::pong, timeout_handler_, &TimeoutHandler::bump);
	connect(socket_, &QWebSocket::binaryMessageReceived, this, &P2PFolder::handleMessage);
	connect(socket_, &QWebSocket::binaryMessageReceived, timeout_handler_, &TimeoutHandler::bump);
	connect(socket_, &QWebSocket::connected, this, &P2PFolder::handleConnected);
	connect(socket_, &QWebSocket::aboutToClose, this, [=]{fgroup_->detach(this);});
	connect(socket_, &QWebSocket::disconnected, this, &P2PFolder::handleDisconnected);
}

void P2PFolder::setConnectedSocket(QWebSocket* socket) {
	resetUnderlyingSocket(socket);

	role_ = Role::SERVER;

	timeout_handler_->start();
	handleConnected();
}

void P2PFolder::open(QUrl url) {
	resetUnderlyingSocket(new QWebSocket(Version().user_agent()));

	role_ = Role::CLIENT;

	timeout_handler_->start();
	socket_->setSslConfiguration(P2PProvider::getSslConfiguration(node_key_));
	socket_->open(url);
}

QByteArray P2PFolder::digest() const {
	return socket_->sslConfiguration().peerCertificate().digest(node_key_->digestAlgorithm());
}

QPair<QHostAddress, quint16> P2PFolder::endpoint() const {
	return {socket_->peerAddress(), socket_->peerPort()};
}

QString P2PFolder::endpointString() const {
	switch(socket_->peerAddress().protocol()) {
		case QAbstractSocket::IPv4Protocol:
			return QString("%1:%2").arg(socket_->peerAddress().toString()).arg(socket_->peerPort());
		case QAbstractSocket::IPv6Protocol:
			return QString("[%1]:%2").arg(socket_->peerAddress().toString()).arg(socket_->peerPort());
		default:
			return "";
	}
}

QString P2PFolder::clientName() const {
	return handshake_handler_->clientName();
}

QString P2PFolder::userAgent() const {
	return handshake_handler_->userAgent();
}

QJsonObject P2PFolder::collectState() {
	QJsonObject state;

	state["endpoint"] = endpointString();   //FIXME: Must be host:port
	state["client_name"] = clientName();
	state["user_agent"] = userAgent();
	state["traffic_stats_all"] = counter_all_.heartbeat_json();
	state["traffic_stats_blocks"] = counter_blocks_.heartbeat_json();
	state["rtt"] = double(ping_handler_->getRtt().count());

	return state;
}

bool P2PFolder::isValid() const {
	return handshake_handler_->isValid();
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
void P2PFolder::sendMessage(QByteArray message) {
	counter_all_.add_up(message.size());
	socket_->sendBinaryMessage(message);
}

void P2PFolder::sendMessage(const blob& message) {
	sendMessage(QByteArray::fromRawData((char*)message.data(), message.size()));
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

	counter_blocks_.add_up(block.size());

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

	counter_all_.add_down(message_raw.size());

	timeout_handler_->bump();

	if(handshake_handler_->isValid()) {
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
		handshake_handler_->handleMesssage(message);
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

	counter_blocks_.add_down(message_struct.content.size());

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

void P2PFolder::handleConnected() {
	ping_handler_->start();

	if(fgroup_->attach(this)) {
		handshake_handler_->handleEstablishedConnection(HandshakeHandler::Role(role_), node_key_->digest(), digest());
	}else
		socket_->close(QWebSocketProtocol::CloseCodePolicyViolated);
}

} /* namespace librevault */
