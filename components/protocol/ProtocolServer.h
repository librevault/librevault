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
#include <SignedMeta.h>
#include <QObject>
#include <exception_helper.hpp>

namespace librevault {
namespace protocol {
namespace v2 {

#define DECLARE_MESSAGE(message_name, fields...) \
  Q_SIGNAL void parsed##message_name(fields);    \
  Q_SLOT void serialize##message_name(fields);

class ProtocolServer : public QObject {
  Q_OBJECT

 public:
  DECLARE_EXCEPTION(ParseError, "Parse error");
  DECLARE_EXCEPTION_DETAIL(MessageTruncated, ParseError, "Message is truncated");

  Q_SIGNAL void serialized(const QByteArray& message_bytes);
  Q_SIGNAL void parseFailed(const QString& reason, const QByteArray& message_bytes);
  Q_SLOT void parse(const QByteArray& message_bytes);

  DECLARE_MESSAGE(Handshake, const QByteArray& auth_token, const QString& device_name, const QString& user_agent,
                  quint16 dht_port);

  DECLARE_MESSAGE(Choke);
  DECLARE_MESSAGE(Unchoke);
  DECLARE_MESSAGE(Interested);
  DECLARE_MESSAGE(NotInterested);

  DECLARE_MESSAGE(IndexUpdate, const MetaInfo::PathRevision& revision, const QBitArray& bitfield);

  DECLARE_MESSAGE(MetaRequest, const MetaInfo::PathRevision& revision);
  DECLARE_MESSAGE(MetaResponse, const SignedMeta& smeta, const QBitArray& bitfield);

  DECLARE_MESSAGE(BlockRequest, const QByteArray& ct_hash, quint64 offset, quint32 length);
  DECLARE_MESSAGE(BlockResponse, const QByteArray& ct_hash, quint64 offset, const QByteArray& content);
};

}  // namespace v2
}  // namespace protocol
}  // namespace librevault
