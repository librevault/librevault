/* Copyright (C) 2015 Alexander Shishenko <alex@shishenko.com>
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
#include "SignedMeta.h"
#include "conv_bitfield.h"
#include <QObject>

#define DECLARE_MESSAGE_HANDLER(message_name, args...) \
Q_SLOT void serialize##message_name(args) const; \
Q_SIGNAL void parsed##message_name(args) const;

namespace librevault {

class ProtocolServer : public QObject {
  Q_OBJECT

public:
  Q_SLOT void parse(const QByteArray& message) const;
  Q_SIGNAL void serialized(const QByteArray& message) const;

  DECLARE_MESSAGE_HANDLER(Handshake, const QByteArray& auth_token, const QString& device_name, const QString& user_agent, quint16 dht_port);
  DECLARE_MESSAGE_HANDLER(IndexUpdate, const QByteArray& path_id, quint64 revision, const QBitArray& bitfield);
  DECLARE_MESSAGE_HANDLER(MetaRequest, const QByteArray& path_id, quint64 revision);
  DECLARE_MESSAGE_HANDLER(MetaResponse, const QByteArray& meta, const QByteArray& signature, const QBitArray& bitfield);
  DECLARE_MESSAGE_HANDLER(BlockRequest, const QByteArray& ct_hash, quint64 offset, quint32 length);
  DECLARE_MESSAGE_HANDLER(BlockResponse, const QByteArray& ct_hash, quint64 offset, const QByteArray& content);
};

} /* namespace librevault */
