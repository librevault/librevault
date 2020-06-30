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
#include <QList>
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
    QByteArray auth_token;
    QString device_name;
    QString user_agent;
    QList<QByteArray> extensions;
  };
  struct HaveMeta {
    Meta::PathRevision revision;
    QBitArray bitfield;
  };
  struct HaveChunk {
    QByteArray ct_hash;
  };
  struct MetaRequest {
    Meta::PathRevision revision;
  };
  struct MetaReply {
    SignedMeta smeta;
    QBitArray bitfield;
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

  MessageType parse_MessageType(const QByteArray& message_raw) {
    if (!message_raw.isEmpty())
      return (MessageType)message_raw[0];
    else
      throw parse_error();
  }

  QByteArray gen_Handshake(const Handshake& message_struct);
  Handshake parse_Handshake(const QByteArray& message_raw);

  QByteArray gen_Choke() { return QByteArray{1, CHOKE}; }
  QByteArray gen_Unchoke() { return QByteArray{1, UNCHOKE}; }
  QByteArray gen_Interested() { return QByteArray{1, INTERESTED}; }
  QByteArray gen_NotInterested() { return QByteArray{1, NOT_INTERESTED}; }

  QByteArray gen_HaveMeta(const HaveMeta& message_struct);
  HaveMeta parse_HaveMeta(const QByteArray& message_raw);

  QByteArray gen_HaveChunk(const HaveChunk& message_struct);
  HaveChunk parse_HaveChunk(const QByteArray& message_raw);

  QByteArray gen_MetaRequest(const MetaRequest& message_struct);
  MetaRequest parse_MetaRequest(const QByteArray& message_raw);

  QByteArray gen_MetaReply(const MetaReply& message_struct);
  MetaReply parse_MetaReply(const QByteArray& message_raw, const Secret& secret_verifier);

  QByteArray gen_BlockRequest(const BlockRequest& message_struct);
  BlockRequest parse_BlockRequest(const QByteArray& message_raw);

  QByteArray gen_BlockReply(const BlockReply& message_struct);
  BlockReply parse_BlockReply(const QByteArray& message_raw);
};

}  // namespace librevault
