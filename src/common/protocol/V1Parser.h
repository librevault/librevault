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
#include "util/conv_bitfield.h"

namespace librevault {

class V1Parser {
 public:
  enum message_type : uint8_t {
    HANDSHAKE = 0,

    CHOKE = 1,
    UNCHOKE = 2,
    INTERESTED = 3,
    NOT_INTERESTED = 4,

    HAVE_META = 5,
    HAVE_CHUNK = 6,

    META_REQUEST = 7,
    META_REPLY = 8,

    BLOCK_REQUEST = 10,
    BLOCK_REPLY = 11,
  };

  const char* type_text(message_type type) {
    switch (type) {
      case HANDSHAKE:
        return "HANDSHAKE";
      case CHOKE:
        return "CHOKE";
      case UNCHOKE:
        return "UNCHOKE";
      case INTERESTED:
        return "INTERESTED";
      case NOT_INTERESTED:
        return "NOT_INTERESTED";
      case HAVE_META:
        return "HAVE_META";
      case HAVE_CHUNK:
        return "HAVE_CHUNK";
      case META_REQUEST:
        return "META_REQUEST";
      case META_REPLY:
        return "META_REPLY";
      case BLOCK_REQUEST:
        return "BLOCK_REQUEST";
      case BLOCK_REPLY:
        return "BLOCK_REPLY";
      default:
        return "UNKNOWN";
    }
  }

  struct Handshake {
    QByteArray auth_token;
    QString device_name;
    QString user_agent;
    std::vector<std::string> extensions;
  };
  struct HaveMeta {
    Meta::PathRevision revision;
    bitfield_type bitfield;
  };
  struct HaveChunk {
    QByteArray ct_hash;
  };
  struct MetaRequest {
    Meta::PathRevision revision;
  };
  struct MetaReply {
    SignedMeta smeta;
    bitfield_type bitfield;
  };
  struct BlockRequest {
    QByteArray ct_hash;
    uint32_t offset;
    uint32_t length;
  };
  struct BlockReply {
    QByteArray ct_hash;
    uint32_t offset;
    QByteArray content;
  };

  /* Errors */
  struct parse_error : std::runtime_error {
    parse_error() : std::runtime_error("Parse error") {}
  };

  // gen_* messages return messages in format <type=byte><payload>
  // parse_* messages take argument in format <type=byte><payload>

  message_type parse_MessageType(const QByteArray& message) {
    if (!message.isEmpty())
      return (message_type)message[0];
    else
      throw parse_error();
  }

  QByteArray gen_Handshake(const Handshake& message_struct);
  Handshake parse_Handshake(const QByteArray& message);

  QByteArray gen_Choke() { return QByteArray{1, CHOKE}; }
  QByteArray gen_Unchoke() { return QByteArray{1, UNCHOKE}; }
  QByteArray gen_Interested() { return QByteArray{1, INTERESTED}; }
  QByteArray gen_NotInterested() { return QByteArray{1, NOT_INTERESTED}; }

  QByteArray gen_HaveMeta(const HaveMeta& message_struct);
  HaveMeta parse_HaveMeta(const QByteArray& message);

  QByteArray gen_HaveChunk(const HaveChunk& message_struct);
  HaveChunk parse_HaveChunk(const QByteArray& message);

  QByteArray gen_MetaRequest(const MetaRequest& message_struct);
  MetaRequest parse_MetaRequest(const QByteArray& message);

  QByteArray gen_MetaReply(const MetaReply& message_struct);
  MetaReply parse_MetaReply(const QByteArray& message, const Secret& secret_verifier);

  QByteArray gen_BlockRequest(const BlockRequest& message_struct);
  BlockRequest parse_BlockRequest(const QByteArray& message);

  QByteArray gen_BlockReply(const BlockReply& message_struct);
  BlockReply parse_BlockReply(const QByteArray& message);

 protected:
  template <class ProtoMessageClass>
  QByteArray prepare_proto_message(const ProtoMessageClass& message_protobuf, message_type type) {
    QByteArray message_raw(message_protobuf.ByteSizeLong() + 1, 0);
    message_raw[0] = type;
    message_protobuf.SerializeToArray(message_raw.data() + 1, message_protobuf.ByteSizeLong());

    return message_raw;
  }
};

}  // namespace librevault
