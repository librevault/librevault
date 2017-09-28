/* Copyright (C) 2014-2017 Alexander Shishenko <alex@shishenko.com>
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
#include "HandshakeHandler.h"
#include "PeerServer.h"
#include "PingHandler.h"
#include "Version.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "nodekey/NodeKey.h"

namespace librevault {

Q_LOGGING_CATEGORY(log_peer, "p2p.peer");

Peer::Peer(const FolderParams& params, NodeKey* node_key,
           BandwidthCounter* bc_all, BandwidthCounter* bc_blocks,
           QObject* parent)
    : QObject(parent),
      node_key_(node_key),
      bc_all_(bc_all),
      bc_blocks_(bc_blocks) {
  qCDebug(log_peer) << "new peer";

  handshake_handler_ = new HandshakeHandler(
      params, Config::get()->getGlobal("client_name").toString(),
      Version().user_agent(), this);
  connect(handshake_handler_, &HandshakeHandler::handshakeSuccess, this,
          &Peer::handshakeSuccess);
  connect(handshake_handler_, &HandshakeHandler::handshakeFailed, this,
          &Peer::handshakeFailed);
  connect(handshake_handler_, &HandshakeHandler::messagePrepared, this,
          &Peer::send);
  connect(this, &Peer::rcvdChoke, [=] { peer_choking_ = true; });
  connect(this, &Peer::rcvdUnchoke, [=] { peer_choking_ = false; });
  connect(this, &Peer::rcvdInterest, [=] { peer_interested_ = true; });
  connect(this, &Peer::rcvdUninterest, [=] { peer_interested_ = false; });

  ping_handler_ = new PingHandler(this);
  timeout_handler_ = new TimeoutHandler(this);
  connect(timeout_handler_, &TimeoutHandler::timedOut, this,
          &Peer::handleDisconnected);

  // Internal signal interconnection
  connect(this, &Peer::handshakeFailed, this, &Peer::handleDisconnected);
}

Peer::~Peer() = default;

QUrl Peer::makeUrl(QPair<QHostAddress, quint16> endpoint, QByteArray folderid) {
  QUrl url;
  url.setScheme("wss");
  url.setPath("/" + folderid.toHex());
  url.setHost(endpoint.first.toString());
  url.setPort(endpoint.second);
  return url;
}

void Peer::resetUnderlyingSocket(QWebSocket* socket) {
  if (socket_) socket_->deleteLater();
  socket_ = socket;
  socket_->setParent(this);

  connect(ping_handler_, &PingHandler::sendPing, socket_, &QWebSocket::ping);
  connect(socket_, &QWebSocket::pong, ping_handler_, &PingHandler::handlePong);
  connect(socket_, &QWebSocket::pong, timeout_handler_, &TimeoutHandler::bump);
  connect(socket_, &QWebSocket::binaryMessageReceived, this, &Peer::handle);
  connect(socket_, &QWebSocket::binaryMessageReceived, timeout_handler_,
          &TimeoutHandler::bump);
  connect(socket_, &QWebSocket::connected, this, &Peer::handleConnected);
  connect(socket_, &QWebSocket::disconnected, this, &Peer::handleDisconnected);
}

void Peer::setConnectedSocket(QWebSocket* socket) {
  resetUnderlyingSocket(socket);

  role_ = Role::SERVER;
  qCDebug(log_peer) << "New incoming connection:" << socket->requestUrl();

  timeout_handler_->start();
  handleConnected();
}

void Peer::open(const QUrl& url) {
  resetUnderlyingSocket(new QWebSocket(Version().user_agent()));

  role_ = Role::CLIENT;
  qCDebug(log_peer) << "New outgoing connection:" << url;

  timeout_handler_->start();
  socket_->setSslConfiguration(node_key_->getSslConfiguration());
  socket_->open(url);
}

QByteArray Peer::digest() const {
  return socket_->sslConfiguration().peerCertificate().digest(
      node_key_->digestAlgorithm());
}

QPair<QHostAddress, quint16> Peer::endpoint() const {
  return {socket_->peerAddress(), socket_->peerPort()};
}

QString Peer::clientName() const { return handshake_handler_->clientName(); }
QString Peer::userAgent() const { return handshake_handler_->userAgent(); }
bool Peer::isValid() const { return handshake_handler_->isValid(); }

std::shared_ptr<InterestGuard> Peer::getInterestGuard() {
  try {
    return std::shared_ptr<InterestGuard>(interest_guard_);
  } catch (std::bad_weak_ptr& e) {
    auto guard = std::make_shared<InterestGuard>(this);
    interest_guard_ = guard;
    return guard;
  }
}

/* RPC Actions */
void Peer::send(const QByteArray& message) {
  bc_all_.add_up(message.size());
  socket_->sendBinaryMessage(message);
}

void Peer::handle(const QByteArray& message) {
  try {
    protocol::v2::Header header;
    QByteArray payload;
    std::tie(header, payload) = parser.parseMessage(message);

    bc_all_.add_down(message.size());
    timeout_handler_->bump();

    if (!handshake_handler_->isValid()) {
      if (header.type != protocol::v2::HANDSHAKE) throw HandshakeExpected();
      return handshake_handler_->handleMesssage(message);
    }

    switch (header.type) {
      case protocol::v2::CHOKE: rcvdChoke(); break;
      case protocol::v2::UNCHOKE: rcvdUnchoke(); break;
      case protocol::v2::INTEREST: rcvdInterest(); break;
      case protocol::v2::UNINTEREST: rcvdUninterest(); break;
      case protocol::v2::INDEXUPDATE:
        rcvdIndexUpdate(parser.parseIndexUpdate(payload));
        break;
      case protocol::v2::METAREQUEST:
        rcvdMetaRequest(parser.parseMetaRequest(payload));
        break;
      case protocol::v2::METARESPONSE:
        rcvdMetaResponse(parser.parseMetaResponse(payload));
        break;
      case protocol::v2::BLOCKREQUEST:
        rcvdBlockRequest(parser.parseBlockRequest(payload));
        break;
      case protocol::v2::BLOCKRESPONSE:
        rcvdBlockResponse(parser.parseBlockResponse(payload));
        break;
      default: throw InvalidMessageType();
    }
  } catch (const HandshakeExpected& e) {
    socket_->close(QWebSocketProtocol::CloseCodePolicyViolated);
  } catch (const HandshakeUnexpected& e) {
    socket_->close(QWebSocketProtocol::CloseCodePolicyViolated);
  } catch (const InvalidMessageType& e) {
    socket_->close(QWebSocketProtocol::CloseCodeProtocolError);
  } catch (const std::exception& e) {
    socket_->close(QWebSocketProtocol::CloseCodeAbnormalDisconnection);
  }
}  // namespace librevault

void Peer::handleConnected() {
  ping_handler_->start();
  handshake_handler_->handleEstablishedConnection(role_, node_key_->digest(),
                                                  digest());
}

/* InterestGuard */
InterestGuard::InterestGuard(Peer* remote) : peer_(remote) {
  peer_->sendInterest();
}

InterestGuard::~InterestGuard() { peer_->sendUninterest(); }

} /* namespace librevault */
