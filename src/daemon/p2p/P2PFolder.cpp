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
#include "P2PFolder.h"

#include "P2PProvider.h"
#include "Tokens.h"
#include "Version.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "folder/FolderService.h"
#include "nodekey/NodeKey.h"
#include "protocol/V1Parser.h"
#include "util/conv_bitarray.h"
#include "util/readable.h"

namespace librevault {

P2PFolder::P2PFolder(QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key, Role role)
    : RemoteFolder(fgroup), role_(role), provider_(provider), node_key_(node_key), socket_(socket), fgroup_(fgroup) {
  LOGFUNC();

  socket_->setParent(this);
  this->setParent(fgroup_);

  // Set up timers
  ping_timer_ = new QTimer(this);
  timeout_timer_ = new QTimer(this);

  // Connect signals
  connect(ping_timer_, &QTimer::timeout, socket_, [this] { socket_->ping(); });
  connect(timeout_timer_, &QTimer::timeout, this, &P2PFolder::deleteLater);
  connect(socket_, &QWebSocket::pong, this, &P2PFolder::handlePong);
  connect(socket_, &QWebSocket::binaryMessageReceived, this, &P2PFolder::handle_message);
  connect(socket_, &QWebSocket::connected, this, &P2PFolder::handleConnected);
  connect(socket_, &QWebSocket::aboutToClose, this, [this] { fgroup_->detach(this); });
  connect(socket_, &QWebSocket::disconnected, this, &P2PFolder::deleteLater);
  connect(this, &RemoteFolder::handshakeFailed, this, &P2PFolder::deleteLater);

  // Start timers
  ping_timer_->start(20 * 1000);

  timeout_timer_->setSingleShot(true);
  bumpTimeout();
}

P2PFolder::P2PFolder(QUrl url, QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key)
    : P2PFolder(socket, fgroup, provider, node_key, CLIENT) {
  socket_->setSslConfiguration(provider_->getSslConfiguration());
  socket_->open(url);
}

P2PFolder::P2PFolder(QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key)
    : P2PFolder(socket, fgroup, provider, node_key, SERVER) {
  handleConnected();
}

P2PFolder::~P2PFolder() {
  LOGFUNC();
  fgroup_->detach(this);
}

QString P2PFolder::displayName() const { return endpoint().toString(); }

QByteArray P2PFolder::digest() const {
  return socket_->sslConfiguration().peerCertificate().digest(node_key_->digestAlgorithm());
}

Endpoint P2PFolder::endpoint() const { return Endpoint(socket_->peerAddress(), socket_->peerPort()); }

QJsonObject P2PFolder::collect_state() {
  QJsonObject state;

  state["endpoint"] = endpoint().toString();  // FIXME: Must be host:port
  state["client_name"] = client_name();
  state["user_agent"] = user_agent();
  state["traffic_stats"] = counter_.heartbeat_json();
  state["rtt"] = double(rtt_.count());

  return state;
}

blob P2PFolder::local_token() { return conv_bytearray(derive_token(fgroup_->secret(), node_key_->digest())); }

blob P2PFolder::remote_token() { return conv_bytearray(derive_token(fgroup_->secret(), digest())); }

void P2PFolder::send_message(const QByteArray& message) {
  counter_.add_up(message.size());
  fgroup_->bandwidth_counter().add_up(message.size());
  socket_->sendBinaryMessage(message);
}

void P2PFolder::sendHandshake() {
  V1Parser::Handshake message_struct;
  message_struct.auth_token = local_token();
  message_struct.device_name = Config::get()->getGlobal("client_name").toString().toStdString();
  message_struct.user_agent = Version::current().userAgent().toStdString();

  send_message(V1Parser().gen_Handshake(message_struct));
  handshake_sent_ = true;
  LOGD("==> HANDSHAKE");
}

/* RPC Actions */
void P2PFolder::choke() {
  if (!am_choking_) {
    send_message(V1Parser().gen_Choke());
    am_choking_ = true;

    LOGD("==> CHOKE");
  }
}
void P2PFolder::unchoke() {
  if (am_choking_) {
    send_message(V1Parser().gen_Unchoke());
    am_choking_ = false;

    LOGD("==> UNCHOKE");
  }
}
void P2PFolder::interest() {
  if (!am_interested_) {
    send_message(V1Parser().gen_Interested());
    am_interested_ = true;

    LOGD("==> INTERESTED");
  }
}
void P2PFolder::uninterest() {
  if (am_interested_) {
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
       << " path_id=" << path_id_readable(message.revision.path_id_) << " revision=" << message.revision.revision_
       << " bits=" << conv_bitarray(message.bitfield));
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
       << " path_id=" << path_id_readable(revision.path_id_) << " revision=" << revision.revision_);
}
void P2PFolder::post_meta(const SignedMeta& smeta, const bitfield_type& bitfield) {
  V1Parser::MetaReply message;
  message.smeta = smeta;
  message.bitfield = bitfield;
  send_message(V1Parser().gen_MetaReply(message));

  LOGD("==> META_REPLY:"
       << " path_id=" << path_id_readable(smeta.meta().path_id()) << " revision=" << smeta.meta().revision()
       << " bits=" << conv_bitarray(bitfield));
}

void P2PFolder::request_block(const blob& ct_hash, uint32_t offset, uint32_t length) {
  V1Parser::BlockRequest message;
  message.ct_hash = ct_hash;
  message.offset = offset;
  message.length = length;
  send_message(V1Parser().gen_BlockRequest(message));

  LOGD("==> BLOCK_REQUEST:"
       << " ct_hash=" << ct_hash_readable(ct_hash) << " offset=" << offset << " length=" << length);
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
       << " ct_hash=" << ct_hash_readable(ct_hash) << " offset=" << offset);
}

void P2PFolder::handle_message(const QByteArray& message) {
  blob message_raw(message.begin(), message.end());
  V1Parser::message_type message_type = V1Parser().parse_MessageType(message_raw);

  counter_.add_down(message_raw.size());
  fgroup_->bandwidth_counter().add_down(message_raw.size());

  bumpTimeout();

  if (ready()) {
    switch (message_type) {
      case V1Parser::CHOKE:
        handle_Choke(message_raw);
        break;
      case V1Parser::UNCHOKE:
        handle_Unchoke(message_raw);
        break;
      case V1Parser::INTERESTED:
        handle_Interested(message_raw);
        break;
      case V1Parser::NOT_INTERESTED:
        handle_NotInterested(message_raw);
        break;
      case V1Parser::HAVE_META:
        handle_HaveMeta(message_raw);
        break;
      case V1Parser::HAVE_CHUNK:
        handle_HaveChunk(message_raw);
        break;
      case V1Parser::META_REQUEST:
        handle_MetaRequest(message_raw);
        break;
      case V1Parser::META_REPLY:
        handle_MetaReply(message_raw);
        break;
      case V1Parser::BLOCK_REQUEST:
        handle_BlockRequest(message_raw);
        break;
      case V1Parser::BLOCK_REPLY:
        handle_BlockReply(message_raw);
        break;
      default:
        socket_->close(QWebSocketProtocol::CloseCodeProtocolError);
    }
  } else {
    handle_Handshake(message_raw);
  }
}

void P2PFolder::handle_Handshake(const blob& message_raw) {
  LOGFUNC();
  try {
    auto message_struct = V1Parser().parse_Handshake(message_raw);
    LOGD("<== HANDSHAKE");

    // Checking authentication using token
    if (message_struct.auth_token != remote_token()) throw auth_error();

    if (role_ == SERVER) sendHandshake();

    client_name_ = QString::fromStdString(message_struct.device_name);
    user_agent_ = QString::fromStdString(message_struct.user_agent);

    LOGD("LV Handshake successful");
    handshake_received_ = true;

    emit handshakeSuccess();
  } catch (std::exception& e) {
    emit handshakeFailed();
  }
}

void P2PFolder::handle_Choke(const blob& message_raw) {
  LOGFUNC();
  LOGD("<== CHOKE");

  if (!peer_choking_) {
    peer_choking_ = true;
    emit rcvdChoke();
  }
}
void P2PFolder::handle_Unchoke(const blob& message_raw) {
  LOGFUNC();
  LOGD("<== UNCHOKE");

  if (peer_choking_) {
    peer_choking_ = false;
    emit rcvdUnchoke();
  }
}
void P2PFolder::handle_Interested(const blob& message_raw) {
  LOGFUNC();
  LOGD("<== INTERESTED");

  if (!peer_interested_) {
    peer_interested_ = true;
    emit rcvdInterested();
  }
}
void P2PFolder::handle_NotInterested(const blob& message_raw) {
  LOGFUNC();
  LOGD("<== NOT_INTERESTED");

  if (peer_interested_) {
    peer_interested_ = false;
    emit rcvdNotInterested();
  }
}

void P2PFolder::handle_HaveMeta(const blob& message_raw) {
  LOGFUNC();

  auto message_struct = V1Parser().parse_HaveMeta(message_raw);
  LOGD("<== HAVE_META:"
       << " path_id=" << path_id_readable(message_struct.revision.path_id_)
       << " revision=" << message_struct.revision.revision_ << " bits=" << conv_bitarray(message_struct.bitfield));

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
       << " revision=" << message_struct.smeta.meta().revision() << " bits=" << conv_bitarray(message_struct.bitfield));

  emit rcvdMetaReply(message_struct.smeta, message_struct.bitfield);
}

void P2PFolder::handle_BlockRequest(const blob& message_raw) {
  LOGFUNC();

  auto message_struct = V1Parser().parse_BlockRequest(message_raw);
  LOGD("<== BLOCK_REQUEST:"
       << " ct_hash=" << ct_hash_readable(message_struct.ct_hash) << " length=" << message_struct.length
       << " offset=" << message_struct.offset);

  emit rcvdBlockRequest(message_struct.ct_hash, message_struct.offset, message_struct.length);
}
void P2PFolder::handle_BlockReply(const blob& message_raw) {
  LOGFUNC();

  auto message_struct = V1Parser().parse_BlockReply(message_raw);
  LOGD("<== BLOCK_REPLY:"
       << " ct_hash=" << ct_hash_readable(message_struct.ct_hash) << " offset=" << message_struct.offset);

  counter_.add_down_blocks(message_struct.content.size());
  fgroup_->bandwidth_counter().add_down_blocks(message_struct.content.size());

  emit rcvdBlockReply(message_struct.ct_hash, message_struct.offset, message_struct.content);
}

void P2PFolder::bumpTimeout() { timeout_timer_->start(120 * 1000); }

void P2PFolder::handlePong(quint64 rtt) {
  bumpTimeout();
  rtt_ = std::chrono::milliseconds(rtt);
}

void P2PFolder::handleConnected() {
  if (!provider_->isLoopback(digest()) && fgroup_->attach(this)) {
    if (role_ == CLIENT) sendHandshake();
  } else
    socket_->close(QWebSocketProtocol::CloseCodePolicyViolated);
}

}  // namespace librevault
