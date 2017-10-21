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
#pragma once

#include "Peer.h"
#include "Secret.h"
#include "control/FolderParams.h"
#include <QObject>

namespace librevault {

class HandshakeHandler : public QObject {
  Q_OBJECT

 public:
  HandshakeHandler(const FolderParams& params, QString client_name, QString user_agent,
      QObject* parent = nullptr);

  Q_SIGNAL void handshakeSuccess();
  Q_SIGNAL void handshakeFailed(QString error);
  Q_SIGNAL void messagePrepared(QByteArray msg);

  Q_SLOT void handleEstablished(
      Peer::Role role, const QByteArray& local_digest, const QByteArray& remote_digest);
  Q_SLOT void handleMesssage(const QByteArray& msg);

  /* Getters */
  QString clientName() const { return remote_client_name_; }
  QString userAgent() const { return remote_user_agent_; }

  bool isValid() const { return handshake_received_; }

 private:
  FolderParams params_;
  Peer::Role role_ = Peer::UNDEFINED;

  /* Handshake fields */
  QString local_client_name_, remote_client_name_;
  QString local_user_agent_, remote_user_agent_;

  /* Digests */
  QByteArray local_digest_, remote_digest_;

  bool handshake_received_ = false, handshake_sent_ = false;

  Q_SLOT void sendHandshake();

  /* Token generators */
  QByteArray localToken();
  QByteArray remoteToken();
};

} /* namespace librevault */
