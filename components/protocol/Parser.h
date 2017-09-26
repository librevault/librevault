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
#pragma once
#include "SignedMeta.h"
#include "conv_bitfield.h"
#include <exception_helper.hpp>

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

namespace librevault {
namespace protocol {
namespace v2 {

enum MessageType : uint8_t {
  HANDSHAKE = 0,

  CHOKE = 1,
  UNCHOKE = 2,
  INTERESTED = 3,
  NOTINTERESTED = 4,

  INDEXUPDATE = 5,

  METAREQUEST = 6,
  METARESPONSE = 7,

  BLOCKREQUEST = 8,
  BLOCKRESPONSE = 9,
};

struct Header {
  MessageType type;
};

struct Handshake {
  QByteArray auth_token;
  QString device_name;
  QString user_agent;
};
struct IndexUpdate {
  MetaInfo::PathRevision revision;
  QBitArray bitfield;
};
struct MetaRequest {
  MetaInfo::PathRevision revision;
};
struct MetaResponse {
  SignedMeta smeta;
  QBitArray bitfield;
};
struct BlockRequest {
  QByteArray ct_hash;
  quint32 offset;
  quint32 length;
};
struct BlockResponse {
  QByteArray ct_hash;
  quint32 offset;
  QByteArray content;
};

class Parser {
 public:
  /* Errors */
  DECLARE_EXCEPTION(ParseError, "Parse error");

  MessageType parse_MessageType(const QByteArray& message_raw) {
    if (message_raw.isEmpty()) throw ParseError();
    return (MessageType)message_raw.at(0);
  }

  std::tuple<Header, QByteArray /*unpacked_payload*/> parseMessage(const QByteArray& message);

  QByteArray serializeHandshake(const Handshake& message_struct);
  Handshake parseHandshake(const QByteArray& payload_bytes);

  QByteArray genChoke() { return serializeMessage(CHOKE); }
  QByteArray genUnchoke() { return serializeMessage(UNCHOKE); }
  QByteArray genInterested() { return serializeMessage(INTERESTED); }
  QByteArray genNotInterested() { return serializeMessage(NOTINTERESTED); }

  QByteArray serializeIndexUpdate(const IndexUpdate& message_struct);
  IndexUpdate parseIndexUpdate(const QByteArray& payload_bytes);

  QByteArray serializeMetaRequest(const MetaRequest& message_struct);
  MetaRequest parseMetaRequest(const QByteArray& payload_bytes);

  QByteArray serializeMetaReply(const MetaResponse& message_struct);
  MetaResponse parseMetaReply(const QByteArray& payload_bytes);

  QByteArray serializeBlockRequest(const BlockRequest& message_struct);
  BlockRequest parseBlockRequest(const QByteArray& payload_bytes);

  QByteArray serializeBlockResponse(const BlockResponse& message_struct);
  BlockResponse parseBlockResponse(const QByteArray& payload_bytes);

 private:
  QByteArray serializeMessage(MessageType type);
  QByteArray serializeMessage(MessageType type, const ::google::protobuf::Message& payload_proto);
};

} /* namespace v2 */
} /* namespace protocol */
} /* namespace librevault */
