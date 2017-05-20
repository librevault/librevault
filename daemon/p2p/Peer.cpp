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
#include "Peer.h"
#include "PeerServer.h"
#include "HandshakeHandler.h"
#include "PingHandler.h"
#include "MessageHandler.h"
#include "Version.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "nodekey/NodeKey.h"
#include "conv_bitarray.h"
#include "V1Parser.h"

namespace librevault {

Peer::Peer(const FolderParams& params, NodeKey* node_key, BandwidthCounter* bc_all, BandwidthCounter* bc_blocks, QObject* parent) :
	QObject(parent),
	node_key_(node_key),
	bc_all_(bc_all),
	bc_blocks_(bc_blocks) {
	qDebug() << "new peer";

	resetUnderlyingSocket(new QWebSocket(Version().user_agent()));

	handshake_handler_ = new HandshakeHandler(params, Config::get()->getGlobal("client_name").toString(), Version().user_agent(), {}, this);
	connect(handshake_handler_, &HandshakeHandler::handshakeSuccess, this, &Peer::handshakeSuccess);
	connect(handshake_handler_, &HandshakeHandler::handshakeFailed, this, &Peer::handshakeFailed);
	connect(handshake_handler_, &HandshakeHandler::messagePrepared, this, &Peer::sendMessage);

	ping_handler_ = new PingHandler();
	timeout_handler_ = new TimeoutHandler();
	connect(timeout_handler_, &TimeoutHandler::timedOut, this, &Peer::handleDisconnected);

	message_handler_ = new MessageHandler(this);

	// Internal signal interconnection
	connect(this, &Peer::handshakeFailed, this, &Peer::handleDisconnected);
}

Peer::~Peer() {}

void Peer::resetUnderlyingSocket(QWebSocket* socket) {
	if(socket_) socket_->deleteLater();
	socket_ = socket;
	socket_->setParent(this);

	connect(ping_handler_, &PingHandler::sendPing, socket_, &QWebSocket::ping);
	connect(socket_, &QWebSocket::pong, ping_handler_, &PingHandler::handlePong);
	connect(socket_, &QWebSocket::pong, timeout_handler_, &TimeoutHandler::bump);
	connect(socket_, &QWebSocket::binaryMessageReceived, this, &Peer::handleMessage);
	connect(socket_, &QWebSocket::binaryMessageReceived, timeout_handler_, &TimeoutHandler::bump);
	connect(socket_, &QWebSocket::connected, this, &Peer::handleConnected);
	connect(socket_, &QWebSocket::disconnected, this, &Peer::handleDisconnected);
}

void Peer::setConnectedSocket(QWebSocket* socket) {
	resetUnderlyingSocket(socket);

	role_ = Role::SERVER;
	qDebug() << "New incoming connection:" << socket->requestUrl();

	timeout_handler_->start();
	handleConnected();
}

void Peer::open(QUrl url) {
	resetUnderlyingSocket(new QWebSocket(Version().user_agent()));

	role_ = Role::CLIENT;
	qDebug() << "New outgoing connection:" << url;

	timeout_handler_->start();
	socket_->setSslConfiguration(node_key_->getSslConfiguration());
	socket_->open(url);
}

QUrl Peer::makeUrl(QPair<QHostAddress, quint16> endpoint, QByteArray folderid) {
	QUrl url;
	url.setScheme("wss");
	url.setPath("/" + folderid.toHex());
	url.setHost(endpoint.first.toString());
	url.setPort(endpoint.second);
	return url;
}

QByteArray Peer::digest() const {
	return socket_->sslConfiguration().peerCertificate().digest(node_key_->digestAlgorithm());
}

QPair<QHostAddress, quint16> Peer::endpoint() const {
	return {socket_->peerAddress(), socket_->peerPort()};
}

QString Peer::endpointString() const {
	switch(socket_->peerAddress().protocol()) {
		case QAbstractSocket::IPv4Protocol:
			return QString("%1:%2").arg(socket_->peerAddress().toString()).arg(socket_->peerPort());
		case QAbstractSocket::IPv6Protocol:
			return QString("[%1]:%2").arg(socket_->peerAddress().toString()).arg(socket_->peerPort());
		default:
			return "";
	}
}

QString Peer::clientName() const {
	return handshake_handler_->clientName();
}

QString Peer::userAgent() const {
	return handshake_handler_->userAgent();
}

QJsonObject Peer::collectState() {
	QJsonObject state;

	state["endpoint"] = endpointString();   //FIXME: Must be host:port
	state["client_name"] = clientName();
	state["user_agent"] = userAgent();
	state["traffic_stats_all"] = bc_all_.heartbeat_json();
	state["traffic_stats_blocks"] = bc_blocks_.heartbeat_json();
	state["rtt"] = double(ping_handler_->getRtt().count());

	return state;
}

bool Peer::isValid() const {
	return handshake_handler_->isValid();
}

/* InterestGuard */
Peer::InterestGuard::InterestGuard(Peer* remote) : remote_(remote) {
	remote_->message_handler_->sendInterested();
}

Peer::InterestGuard::~InterestGuard() {
	remote_->message_handler_->sendNotInterested();
}

std::shared_ptr<Peer::InterestGuard> Peer::get_interest_guard() {
	try {
		return std::shared_ptr<InterestGuard>(interest_guard_);
	}catch(std::bad_weak_ptr& e){
		auto guard = std::make_shared<InterestGuard>(this);
		interest_guard_ = guard;
		return guard;
	}
}

/* RPC Actions */
void Peer::sendMessage(QByteArray message) {
	bc_all_.add_up(message.size());
	socket_->sendBinaryMessage(message);
}

void Peer::handleMessage(const QByteArray& message) {
	V1Parser::message_type message_type = V1Parser().parse_MessageType(message);

	bc_all_.add_down(message.size());

	timeout_handler_->bump();

	if(handshake_handler_->isValid()) {
		switch(message_type) {
			case V1Parser::CHOKE: message_handler_->handleChoke(message); break;
			case V1Parser::UNCHOKE: message_handler_->handleUnchoke(message); break;
			case V1Parser::INTERESTED: message_handler_->handleInterested(message); break;
			case V1Parser::NOT_INTERESTED: message_handler_->handleNotInterested(message); break;
			case V1Parser::HAVE_META: message_handler_->handleHaveMeta(message); break;
			case V1Parser::HAVE_CHUNK: message_handler_->handleHaveChunk(message); break;
			case V1Parser::META_REQUEST: message_handler_->handleMetaRequest(message); break;
			case V1Parser::META_REPLY: message_handler_->handleMetaReply(message); break;
			case V1Parser::META_CANCEL: message_handler_->handleMetaCancel(message); break;
			case V1Parser::BLOCK_REQUEST: message_handler_->handleBlockRequest(message); break;
			case V1Parser::BLOCK_REPLY: message_handler_->handleBlockReply(message); break;
			case V1Parser::BLOCK_CANCEL: message_handler_->handleBlockCancel(message); break;
			default: socket_->close(QWebSocketProtocol::CloseCodeProtocolError);
		}
	}else{
		handshake_handler_->handleMesssage(message);
	}
}

void Peer::handleConnected() {
	ping_handler_->start();
	handshake_handler_->handleEstablishedConnection(HandshakeHandler::Role(role_), node_key_->digest(), digest());
}

} /* namespace librevault */
