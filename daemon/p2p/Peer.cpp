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
Q_LOGGING_CATEGORY(log_peer_msg, "p2p.peer.msg");

Peer::Peer(const FolderParams& params, NodeKey* node_key, BandwidthCounter* bc_all,
    BandwidthCounter* bc_blocks, QObject* parent)
    : QObject(parent), node_key_(node_key), bc_all_(bc_all), bc_blocks_(bc_blocks) {
  qCDebug(log_peer) << "new peer";

  handshake_handler_ = new HandshakeHandler(
      params, Config::get()->getGlobals().client_name, Version().userAgent(), this);
  connect(handshake_handler_, &HandshakeHandler::handshakeSuccess, this, &Peer::handshakeSuccess);
  connect(handshake_handler_, &HandshakeHandler::messagePrepared, this, &Peer::send);

  ping_handler_ = new PingHandler(this);
  timeout_handler_ = new TimeoutHandler(this);
  connect(timeout_handler_, &TimeoutHandler::timedOut, this, &Peer::disconnected);

  // Internal signal interconnection
  connect(this, &Peer::connected, this, &Peer::handleConnected);
}

Peer::~Peer() { qCDebug(log_peer) << "Peer is going to be disconnected:" << endpoint(); }

QUrl Peer::makeUrl(const Endpoint& endpoint, const QByteArray& folderid) {
  QUrl url;
  url.setScheme("wss");
  url.setPath("/" + folderid.toHex());
  url.setHost("[" + endpoint.addr.toString() + "]");
  url.setPort(endpoint.port);
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
  connect(socket_, &QWebSocket::binaryMessageReceived, timeout_handler_, &TimeoutHandler::bump);
  connect(socket_, &QWebSocket::connected, this, &Peer::connected);
  connect(socket_, &QWebSocket::disconnected, this, &Peer::disconnected);
}

void Peer::setConnectedSocket(QWebSocket* socket) {
  resetUnderlyingSocket(socket);

  role_ = Role::SERVER;
  qCDebug(log_peer) << "<=== connection:" << endpoint();

  timeout_handler_->start();
  handleConnected();
}

void Peer::open(const QUrl& url) {
  resetUnderlyingSocket(new QWebSocket(Version::current().userAgent()));

  role_ = Role::CLIENT;
  qCDebug(log_peer) << "===> connection:" << url;

  timeout_handler_->start();
  socket_->setSslConfiguration(node_key_->getSslConfiguration());
  socket_->open(url);
}

QByteArray Peer::digest() const {
  return socket_->sslConfiguration().peerCertificate().digest(node_key_->digestAlgorithm());
}

Endpoint Peer::endpoint() const { return {socket_->peerAddress(), socket_->peerPort()}; }
QString Peer::clientName() const { return handshake_handler_->clientName(); }
QString Peer::userAgent() const { return handshake_handler_->userAgent(); }
bool Peer::isValid() const { return handshake_handler_->isValid(); }

std::shared_ptr<StateGuard> Peer::getInterestGuard() {
  return StateGuard::get(this, interest_guard_, MessageType::INTEREST, MessageType::UNINTEREST);
}

std::shared_ptr<StateGuard> Peer::getUnchokeGuard() {
  return StateGuard::get(this, unchoke_guard_, MessageType::UNCHOKE, MessageType::CHOKE);
}

// RPC Actions
void Peer::send(const protocol::v2::Message& message) {
  try {
    qCDebug(log_peer_msg) << "===>" << message;

    QByteArray message_bytes = parser_.serialize(message);

    bc_all_.addUp(message_bytes.size());
    if (message.header.type == MessageType::BLOCKRESPONSE)
      bc_blocks_.addUp(message.blockresponse.content.size());

    switch (message.header.type) {
      case MessageType::HANDSHAKE: break;
      case MessageType::CHOKE: am_choking_ = true; break;
      case MessageType::UNCHOKE: am_choking_ = false; break;
      case MessageType::INTEREST: am_interested_ = true; break;
      case MessageType::UNINTEREST: am_interested_ = false; break;
      case MessageType::INDEXUPDATE: break;
      case MessageType::METAREQUEST: break;
      case MessageType::METARESPONSE: break;
      case MessageType::BLOCKREQUEST: break;
      case MessageType::BLOCKRESPONSE:
        bc_blocks_.addUp(message.blockresponse.content.size());
        break;
      default: throw InvalidMessageType();
    }

    socket_->sendBinaryMessage(message_bytes);
  } catch (const std::exception& e) {
    qCDebug(log_peer_msg) << "=X=>" << message << "E:" << e.what();
  }
}

void Peer::handle(const QByteArray& message_bytes) {
  protocol::v2::Message message;

  try {
    message = parser_.parse(message_bytes);
    qCDebug(log_peer_msg) << "<===" << message;
    bc_all_.addDown(message_bytes.size());

    if (!handshake_handler_->isValid())
      if (message.header.type == MessageType::HANDSHAKE)
        return handshake_handler_->handlePayload(message.handshake);
      else
        throw HandshakeExpected();
    else if (message.header.type == MessageType::HANDSHAKE)
      throw HandshakeUnexpected();

    switch (message.header.type) {
      case MessageType::CHOKE: peer_choking_ = true; break;
      case MessageType::UNCHOKE: peer_choking_ = false; break;
      case MessageType::INTEREST: peer_interested_ = true; break;
      case MessageType::UNINTEREST: peer_interested_ = false; break;
      case MessageType::INDEXUPDATE: break;
      case MessageType::METAREQUEST: break;
      case MessageType::METARESPONSE: break;
      case MessageType::BLOCKREQUEST: break;
      case MessageType::BLOCKRESPONSE:
        bc_blocks_.addDown(message.blockresponse.content.size());
        break;
      default: throw InvalidMessageType();
    }

    emit received(message);
  } catch (const protocol::v2::Parser::ParseError& e) {
    qCWarning(log_peer_msg) << "<=X=" << message << "E:" << e.what();
    socket_->close(QWebSocketProtocol::CloseCodePolicyViolated);
  } catch (const HandshakeHandler::HandshakeError& e) {
    qCWarning(log_peer_msg) << "<=X=" << message << "E:" << e.what();
    socket_->close(QWebSocketProtocol::CloseCodePolicyViolated);
  } catch (const HandshakeExpected& e) {
    qCWarning(log_peer_msg) << "<=X=" << message << "E:" << e.what();
    socket_->close(QWebSocketProtocol::CloseCodePolicyViolated);
  } catch (const HandshakeUnexpected& e) {
    qCWarning(log_peer_msg) << "<=X=" << message << "E:" << e.what();
    socket_->close(QWebSocketProtocol::CloseCodePolicyViolated);
  } catch (const InvalidMessageType& e) {
    qCWarning(log_peer_msg) << "<=X=" << message << "E:" << e.what();
    socket_->close(QWebSocketProtocol::CloseCodeBadOperation);
  } catch (const std::exception& e) {
    qCWarning(log_peer_msg) << "<=X=" << message << "E:" << e.what();
    socket_->close(QWebSocketProtocol::CloseCodeBadOperation);
  }
}

void Peer::handleConnected() {
  ping_handler_->start();
  handshake_handler_->handleEstablished(role_, node_key_->digest(), digest());
}

/* StateGuard */
StateGuard::StateGuard(Peer* peer, MessageType enter_state, MessageType exit_state)
    : peer_(peer), enter_state_(enter_state), exit_state_(exit_state) {
  protocol::v2::Message msg;
  msg.header.type = enter_state_;
  peer_->send(msg);
}
StateGuard::~StateGuard() {
  protocol::v2::Message msg;
  msg.header.type = exit_state_;
  peer_->send(msg);
}

std::shared_ptr<StateGuard> StateGuard::get(
    Peer* peer, std::weak_ptr<StateGuard>& var, MessageType enter_state, MessageType exit_state) {
  try {
    return std::shared_ptr<StateGuard>(var);
  } catch (const std::bad_weak_ptr& e) {
    auto guard = std::make_shared<StateGuard>(peer, enter_state, exit_state);
    var = guard;
    return guard;
  }
}

}  // namespace librevault
