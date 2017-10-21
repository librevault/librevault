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
#pragma once
#include "MetaInfo.h"
#include "SignedMeta.h"
#include "TimeoutHandler.h"
#include "util/BandwidthCounter.h"
#include "util/Endpoint.h"
#include "v2/Parser.h"
#include <QObject>
#include <QTimer>
#include <QWebSocket>
#include <memory>
#include "util/exception.hpp"

namespace librevault {

class FolderGroup;
class NodeKey;
class HandshakeHandler;
class PingHandler;
class FolderParams;
class StateGuard;

class Peer : public QObject {
  Q_OBJECT

 public:
  Peer(const FolderParams& params, NodeKey* node_key, BandwidthCounter* bc_all,
      BandwidthCounter* bc_blocks, QObject* parent);
  ~Peer() override;

  DECLARE_EXCEPTION(HandshakeExpected, "Handshake message expected");
  DECLARE_EXCEPTION(HandshakeUnexpected, "Handshake message inside an already valid stream");
  DECLARE_EXCEPTION(InvalidMessageType, "Invalid message type received");

  enum Role { UNDEFINED, SERVER, CLIENT };

  static QUrl makeUrl(const Endpoint& endpoint, const QByteArray& folderid);

  Q_SIGNAL void connected();
  Q_SIGNAL void disconnected();
  Q_SIGNAL void handshakeSuccess();

  Q_SLOT void setConnectedSocket(QWebSocket* socket);
  Q_SLOT void open(const QUrl& url);

  /* Messages */
  Q_SLOT void send(const protocol::v2::Message& message);
  Q_SIGNAL void received(const protocol::v2::Message& message);

  /* Getters */
  bool amChoking() const { return am_choking_; }
  bool amInterested() const { return am_interested_; }
  bool peerChoking() const { return peer_choking_; }
  bool peerInterested() const { return peer_interested_; }

  QByteArray digest() const;
  Endpoint endpoint() const;
  QString clientName() const;
  QString userAgent() const;

  bool isValid() const;

  std::shared_ptr<StateGuard> getInterestGuard();
  std::shared_ptr<StateGuard> getUnchokeGuard();

 private:
  NodeKey* node_key_;
  QWebSocket* socket_ = nullptr;

  BandwidthCounter bc_all_, bc_blocks_;

  PingHandler* ping_handler_;
  TimeoutHandler* timeout_handler_;
  HandshakeHandler* handshake_handler_;

  protocol::v2::Parser parser_;

  Role role_ = Role::UNDEFINED;

  bool am_choking_ = true;
  bool am_interested_ = false;
  bool peer_choking_ = true;
  bool peer_interested_ = false;
  std::weak_ptr<StateGuard> interest_guard_, unchoke_guard_;

  // Underlying socket management
  Q_SLOT void resetUnderlyingSocket(QWebSocket* socket);

  Q_SLOT void handleConnected();
  Q_SLOT void handle(const QByteArray& message_bytes);

  std::shared_ptr<StateGuard> getStateGuard(std::weak_ptr<StateGuard>& var,
      protocol::v2::MessageType enter_state, protocol::v2::MessageType exit_state);
};

struct StateGuard {
  explicit StateGuard(
      Peer* remote, protocol::v2::MessageType enter_state, protocol::v2::MessageType exit_state);
  ~StateGuard();

 private:
  Peer* peer_;
  protocol::v2::MessageType enter_state_, exit_state_;
};

}  // namespace librevault
