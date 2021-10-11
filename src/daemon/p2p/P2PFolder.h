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
 */
#pragma once
#include <util/Endpoint.h>

#include <QTimer>
#include <QWebSocket>
#include <chrono>

#include "folder/RemoteFolder.h"
#include "p2p/BandwidthCounter.h"

namespace librevault {

class FolderGroup;
class NodeKey;
class P2PProvider;

class P2PFolder : public RemoteFolder {
  Q_OBJECT
  friend class P2PProvider;

 public:
  /* Errors */
  struct error : public std::runtime_error {
    error(const char* what) : std::runtime_error(what) {}
  };
  struct protocol_error : public error {
    protocol_error() : error("Protocol error") {}
  };
  struct auth_error : public error {
    auth_error() : error("Remote node couldn't verify its authenticity") {}
  };

  P2PFolder(QUrl url, QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key);
  P2PFolder(QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key);
  ~P2PFolder();

  /* Getters */
  QString displayName() const;
  QByteArray digest() const;
  Endpoint endpoint() const;
  QString client_name() const { return client_name_; }
  QString user_agent() const { return user_agent_; }
  QJsonObject collect_state();

  void send_message(const QByteArray& message);

  // Handshake
  void sendHandshake();
  bool ready() const { return handshake_sent_ && handshake_received_; }

  /* Message senders */
  void choke();
  void unchoke();
  void interest();
  void uninterest();

  void post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield);
  void post_have_chunk(const QByteArray& ct_hash);

  void request_meta(const Meta::PathRevision& revision);
  void post_meta(const SignedMeta& smeta, const bitfield_type& bitfield);

  void request_block(const QByteArray& ct_hash, uint32_t offset, uint32_t size);
  void post_block(const QByteArray& ct_hash, uint32_t offset, const QByteArray& block);

 private:
  enum Role { SERVER, CLIENT } role_;

  P2PFolder(QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key, Role role);

  P2PProvider* provider_;
  NodeKey* node_key_;
  QWebSocket* socket_;
  FolderGroup* fgroup_;

  /* Handshake */
  bool handshake_received_ = false;
  bool handshake_sent_ = false;

  BandwidthCounter counter_;

  /* These needed primarily for UI */
  QString client_name_;
  QString user_agent_;

  /* Ping/pong and timeout handlers */
  QTimer* ping_timer_;
  QTimer* timeout_timer_;

  /* Token generators */
  QByteArray local_token();
  QByteArray remote_token();

  void bumpTimeout();

  std::chrono::milliseconds rtt_ = std::chrono::milliseconds(0);

  /* Message handlers */
  void handle_message(const QByteArray& message);

  void handle_Handshake(const QByteArray& message);

  void handle_Choke(const QByteArray& message);
  void handle_Unchoke(const QByteArray& message);
  void handle_Interested(const QByteArray& message);
  void handle_NotInterested(const QByteArray& message);

  void handle_HaveMeta(const QByteArray& message);
  void handle_HaveChunk(const QByteArray& message);

  void handle_MetaRequest(const QByteArray& message);
  void handle_MetaReply(const QByteArray& message);

  void handle_BlockRequest(const QByteArray& message);
  void handle_BlockReply(const QByteArray& message);

 private slots:
  void handlePong(quint64 rtt);
  void handleConnected();
};

}  // namespace librevault
