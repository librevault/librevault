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
#include "V1Parser.h"

#include "V1Protocol.pb.h"

namespace librevault {

QByteArray V1Parser::gen_Handshake(const Handshake& message_struct) {
  protocol::Handshake message_protobuf;
  message_protobuf.set_auth_token(message_struct.auth_token.toStdString());
  message_protobuf.set_device_name(message_struct.device_name.toStdString());
  message_protobuf.set_user_agent(message_struct.user_agent.toStdString());

  for (auto& extension : message_struct.extensions) message_protobuf.add_extensions(extension);

  return prepare_proto_message(message_protobuf, HANDSHAKE);
}
V1Parser::Handshake V1Parser::parse_Handshake(const QByteArray& message) {
  protocol::Handshake message_protobuf;
  if (!message_protobuf.ParseFromArray(message.data() + 1, message.size() - 1)) throw parse_error();

  Handshake message_struct;
  message_struct.auth_token = QByteArray::fromStdString(message_protobuf.auth_token());
  message_struct.device_name = QByteArray::fromStdString(message_protobuf.device_name());
  message_struct.user_agent = QByteArray::fromStdString(message_protobuf.user_agent());

  message_struct.extensions.reserve(message_protobuf.extensions().size());
  for (auto& extension : message_protobuf.extensions()) message_struct.extensions.push_back(extension);

  return message_struct;
}

QByteArray V1Parser::gen_HaveMeta(const HaveMeta& message_struct) {
  protocol::HaveMeta message_protobuf;

  message_protobuf.set_path_id(message_struct.revision.path_id_.toStdString());
  message_protobuf.set_revision(message_struct.revision.revision_);

  std::vector<uint8_t> converted_bitfield = convert_bitfield(message_struct.bitfield);
  message_protobuf.set_bitfield(converted_bitfield.data(), converted_bitfield.size());

  return prepare_proto_message(message_protobuf, HAVE_META);
}
V1Parser::HaveMeta V1Parser::parse_HaveMeta(const QByteArray& message) {
  protocol::HaveMeta message_protobuf;
  if (!message_protobuf.ParseFromArray(message.data() + 1, message.size() - 1)) throw parse_error();

  HaveMeta message_struct;
  message_struct.revision.revision_ = message_protobuf.revision();
  message_struct.revision.path_id_ =
      QByteArray::fromStdString(message_protobuf.path_id());
  message_struct.bitfield =
      convert_bitfield(std::vector<uint8_t>(message_protobuf.bitfield().begin(), message_protobuf.bitfield().end()));

  return message_struct;
}

QByteArray V1Parser::gen_HaveChunk(const HaveChunk& message_struct) {
  protocol::HaveChunk message_protobuf;

  message_protobuf.set_ct_hash(message_struct.ct_hash.toStdString());

  return prepare_proto_message(message_protobuf, HAVE_CHUNK);
}
V1Parser::HaveChunk V1Parser::parse_HaveChunk(const QByteArray& message) {
  protocol::HaveChunk message_protobuf;
  if (!message_protobuf.ParseFromArray(message.data() + 1, message.size() - 1)) throw parse_error();

  HaveChunk message_struct;
  message_struct.ct_hash = QByteArray::fromStdString(message_protobuf.ct_hash());

  return message_struct;
}

QByteArray V1Parser::gen_MetaRequest(const MetaRequest& message_struct) {
  protocol::MetaRequest message_protobuf;
  message_protobuf.set_path_id(message_struct.revision.path_id_.toStdString());
  message_protobuf.set_revision(message_struct.revision.revision_);

  return prepare_proto_message(message_protobuf, META_REQUEST);
}
V1Parser::MetaRequest V1Parser::parse_MetaRequest(const QByteArray& message) {
  protocol::MetaRequest message_protobuf;
  if (!message_protobuf.ParseFromArray(message.data() + 1, message.size() - 1)) throw parse_error();

  MetaRequest message_struct;
  message_struct.revision.path_id_ = QByteArray::fromStdString(message_protobuf.path_id());
  message_struct.revision.revision_ = message_protobuf.revision();

  return message_struct;
}

QByteArray V1Parser::gen_MetaReply(const MetaReply& message_struct) {
  protocol::MetaReply message_protobuf;
  message_protobuf.set_meta(message_struct.smeta.raw_meta().toStdString());
  message_protobuf.set_signature(message_struct.smeta.signature().toStdString());

  std::vector<uint8_t> converted_bitfield = convert_bitfield(message_struct.bitfield);
  message_protobuf.set_bitfield(converted_bitfield.data(), converted_bitfield.size());

  return prepare_proto_message(message_protobuf, META_REPLY);
}
V1Parser::MetaReply V1Parser::parse_MetaReply(const QByteArray& message, const Secret& secret_verifier) {
  protocol::MetaReply message_protobuf;
  if (!message_protobuf.ParseFromArray(message.data() + 1, message.size() - 1)) throw parse_error();

  bitfield_type converted_bitfield =
      convert_bitfield(std::vector<uint8_t>(message_protobuf.bitfield().begin(), message_protobuf.bitfield().end()));

  return MetaReply{SignedMeta(QByteArray::fromStdString(message_protobuf.meta()), QByteArray::fromStdString(message_protobuf.signature()), secret_verifier),
                   std::move(converted_bitfield)};
}

QByteArray V1Parser::gen_BlockRequest(const BlockRequest& message_struct) {
  protocol::BlockRequest message_protobuf;
  message_protobuf.set_ct_hash(message_struct.ct_hash.toStdString());
  message_protobuf.set_offset(message_struct.offset);
  message_protobuf.set_length(message_struct.length);

  return prepare_proto_message(message_protobuf, BLOCK_REQUEST);
}
V1Parser::BlockRequest V1Parser::parse_BlockRequest(const QByteArray& message) {
  protocol::BlockRequest message_protobuf;
  if (!message_protobuf.ParseFromArray(message.data() + 1, message.size() - 1)) throw parse_error();

  BlockRequest message_struct;
  message_struct.ct_hash = QByteArray::fromStdString(message_protobuf.ct_hash());
  message_struct.offset = message_protobuf.offset();
  message_struct.length = message_protobuf.length();

  return message_struct;
}

QByteArray V1Parser::gen_BlockReply(const BlockReply& message_struct) {
  protocol::BlockReply message_protobuf;
  message_protobuf.set_ct_hash(message_struct.ct_hash.toStdString());
  message_protobuf.set_offset(message_struct.offset);
  message_protobuf.set_content(message_struct.content.toStdString());

  return prepare_proto_message(message_protobuf, BLOCK_REPLY);
}
V1Parser::BlockReply V1Parser::parse_BlockReply(const QByteArray& message) {
  protocol::BlockReply message_protobuf;
  if (!message_protobuf.ParseFromArray(message.data() + 1, message.size() - 1)) throw parse_error();

  BlockReply message_struct;
  message_struct.ct_hash = QByteArray::fromStdString(message_protobuf.ct_hash());
  message_struct.offset = message_protobuf.offset();
  message_struct.content = QByteArray::fromStdString(message_protobuf.content());

  return message_struct;
}

}  // namespace librevault
