/* Copyright (C) 2017 Alexander Shishenko <alex@shishenko.com>
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
#include "HandshakeHandler.h"
#include "Tokens.h"
#include "nodekey/NodeKey.h"
#include "v2/Parser.h"

namespace librevault {

HandshakeHandler::HandshakeHandler(const FolderParams& params,
                                   QString client_name, QString user_agent,
                                   QObject* parent)
    : QObject(parent),
      params_(params),
      local_client_name_(client_name),
      local_user_agent_(user_agent) {
  connect(this, &HandshakeHandler::handshakeSuccess,
          [=] { qDebug() << "Librevault handshake successful"; });
  connect(this, &HandshakeHandler::handshakeFailed, [=](QString error) {
    qDebug() << "Librevault handshake failed: " + error;
  });
}

QByteArray HandshakeHandler::localToken() {
  return derive_token(params_.secret, local_digest_);
}

QByteArray HandshakeHandler::remoteToken() {
  return derive_token(params_.secret, remote_digest_);
}

void HandshakeHandler::sendHandshake() {
  Q_ASSERT(!handshake_sent_);

  protocol::v2::Handshake message_struct;
  message_struct.auth_token = localToken();
  message_struct.device_name = local_client_name_;
  message_struct.user_agent = local_user_agent_;

  messagePrepared(protocol::v2::Parser().genHandshake(message_struct));
  handshake_sent_ = true;
  // LOGD("==> HANDSHAKE");
}

void HandshakeHandler::handleEstablished(Peer::Role role,
                                         const QByteArray& local_digest,
                                         const QByteArray& remote_digest) {
  Q_ASSERT(role_ != Peer::UNDEFINED);

  role_ = role;
  local_digest_ = local_digest;
  remote_digest_ = remote_digest;

  if (role_ == Peer::CLIENT) sendHandshake();
}

void HandshakeHandler::handleMesssage(const QByteArray& msg) {
  Q_ASSERT(!handshake_received_);
  Q_ASSERT(role_ != Peer::UNDEFINED);

  if (role_ == Peer::CLIENT && !handshake_sent_) {
    // We are expected to send handshake first
    emit handshakeFailed(
        "Unexpected handshake message from server (client must send its message "
        "first)");
    return;
  }

  QByteArray rcvd_remote_token;
  try {
    auto message_struct = protocol::v2::Parser().parseHandshake(msg);

    remote_client_name_ = message_struct.device_name;
    remote_user_agent_ = message_struct.user_agent;
    rcvd_remote_token = message_struct.auth_token;
  } catch (std::exception& e) {
    emit handshakeFailed(QStringLiteral("Handshake message parse error: ") +
                         e.what());
    return;
  }

  // Checking authentication using token
  if (rcvd_remote_token != remoteToken()) {
    emit handshakeFailed("Remote authentication token is invalid");
    return;
  }

  handshake_received_ = true;

  emit handshakeSuccess();

  if (role_ == Peer::SERVER) sendHandshake();
}

} /* namespace librevault */
