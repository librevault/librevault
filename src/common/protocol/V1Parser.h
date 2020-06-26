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
#include <magic_enum.hpp>

#include "SignedMeta.h"
#include "util/conv_bitfield.h"

namespace librevault {

class V1Parser {
 public:
  enum MessageType : uint8_t {
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

  struct Handshake {
    std::vector<uint8_t> auth_token;
    std::string device_name;
    std::string user_agent;
    std::vector<std::string> extensions;
  };
  struct HaveMeta {
    Meta::PathRevision revision;
    bitfield_type bitfield;
  };
  struct HaveChunk {
    std::vector<uint8_t> ct_hash;
  };
  struct MetaRequest {
    Meta::PathRevision revision;
  };
  struct MetaReply {
    SignedMeta smeta;
    bitfield_type bitfield;
  };
  struct BlockRequest {
    std::vector<uint8_t> ct_hash;
    uint32_t offset;
    uint32_t length;
  };
  struct BlockReply {
    std::vector<uint8_t> ct_hash;
    uint32_t offset;
    std::vector<uint8_t> content;
  };

  /* Errors */
  struct parse_error : std::runtime_error {
    parse_error() : std::runtime_error("Parse error") {}
  };

  // gen_* messages return messages in format <type=byte><payload>
  // parse_* messages take argument in format <type=byte><payload>

  MessageType parse_MessageType(const std::vector<uint8_t>& message_raw) {
    if (!message_raw.empty())
      return (MessageType)message_raw[0];
    else
      throw parse_error();
  }

  QByteArray gen_Handshake(const Handshake& message_struct);
  Handshake parse_Handshake(const std::vector<uint8_t>& message_raw);

  QByteArray gen_Choke() { return QByteArray{1, CHOKE}; }
  QByteArray gen_Unchoke() { return QByteArray{1, UNCHOKE}; }
  QByteArray gen_Interested() { return QByteArray{1, INTERESTED}; }
  QByteArray gen_NotInterested() { return QByteArray{1, NOT_INTERESTED}; }

  QByteArray gen_HaveMeta(const HaveMeta& message_struct);
  HaveMeta parse_HaveMeta(const std::vector<uint8_t>& message_raw);

  QByteArray gen_HaveChunk(const HaveChunk& message_struct);
  HaveChunk parse_HaveChunk(const std::vector<uint8_t>& message_raw);

  QByteArray gen_MetaRequest(const MetaRequest& message_struct);
  MetaRequest parse_MetaRequest(const std::vector<uint8_t>& message_raw);

  QByteArray gen_MetaReply(const MetaReply& message_struct);
  MetaReply parse_MetaReply(const std::vector<uint8_t>& message_raw, const Secret& secret_verifier);

  QByteArray gen_BlockRequest(const BlockRequest& message_struct);
  BlockRequest parse_BlockRequest(const std::vector<uint8_t>& message_raw);

  QByteArray gen_BlockReply(const BlockReply& message_struct);
  BlockReply parse_BlockReply(const std::vector<uint8_t>& message_raw);

 protected:
  template <class ProtoMessageClass>
  QByteArray prepare_proto_message(const ProtoMessageClass& message_protobuf, MessageType type) {
    QByteArray message_raw(message_protobuf.ByteSize() + 1, 0);
    message_raw[0] = type;
    message_protobuf.SerializeToArray(message_raw.data() + 1, message_protobuf.ByteSize());

    return message_raw;
  }
};

}  // namespace librevault
