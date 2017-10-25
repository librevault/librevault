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
#include "tokens.h"
#include "nodekey/NodeKey.h"

namespace librevault {

Q_LOGGING_CATEGORY(log_handshake, "p2p.handshake")

HandshakeHandler::HandshakeHandler(
    const FolderParams& params, QString client_name, QString user_agent, QObject* parent)
    : QObject(parent),
      params_(params),
      local_client_name_(std::move(client_name)),
      local_user_agent_(std::move(user_agent)) {}

QByteArray HandshakeHandler::localToken() const {
  return deriveToken(params_.secret, local_digest_);
}
QByteArray HandshakeHandler::remoteToken() const {
  return deriveToken(params_.secret, remote_digest_);
}

void HandshakeHandler::sendHandshake() {
  Q_ASSERT(!handshake_sent_);

  protocol::v2::Message message;
  message.header.type = protocol::v2::MessageType::HANDSHAKE;
  message.handshake.auth_token = localToken();
  message.handshake.device_name = local_client_name_;
  message.handshake.user_agent = local_user_agent_;

  messagePrepared(message);
  handshake_sent_ = true;
}

void HandshakeHandler::handleEstablished(
    Peer::Role role, const QByteArray& local_digest, const QByteArray& remote_digest) {
  role_ = role;
  Q_ASSERT(role_ != Peer::UNDEFINED);

  local_digest_ = local_digest;
  remote_digest_ = remote_digest;

  if (role_ == Peer::CLIENT) sendHandshake();
}

void HandshakeHandler::handlePayload(const protocol::v2::Message::Handshake& payload) {
  Q_ASSERT(!handshake_received_);
  Q_ASSERT(role_ != Peer::UNDEFINED);

  try {
    // We are expected to send handshake first
    if (role_ == Peer::CLIENT && !handshake_sent_) throw HandshakeOrderingError();

    remote_client_name_ = payload.device_name;
    remote_user_agent_ = payload.user_agent;
    QByteArray rcvd_remote_token = payload.auth_token;

    // Checking authentication using token
    if (rcvd_remote_token != remoteToken()) throw InvalidTokenError();

    handshake_received_ = true;

    if (role_ == Peer::SERVER) sendHandshake();

    qCDebug(log_handshake) << "Librevault handshake successful";
    emit handshakeSuccess();
  } catch (const HandshakeError& e) {
    qCDebug(log_handshake) << "Librevault handshake failed:" << e.what();
    throw;
  }
}

} /* namespace librevault */
