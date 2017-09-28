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
#include "Parser.h"
#include "protocol_convert.h"
#include <messages_v2.pb.h>
#include <QDataStream>
#include <QDebug>
#include <QLoggingCategory>

namespace librevault {
namespace protocol {
namespace v2 {

Q_LOGGING_CATEGORY(log_protocol, "protocol.v2");

namespace {

void prepareDataStream(QDataStream& stream) {
  stream.setByteOrder(QDataStream::BigEndian);
  stream.setVersion(QDataStream::Qt_5_0);
}

QDataStream& operator<<(QDataStream& stream, const ::google::protobuf::Message& message_proto) {
  QByteArray message_bytes(message_proto.ByteSize(), 0);
  message_proto.SerializeToArray(message_bytes.data(), message_bytes.size());
  stream << message_bytes;
  return stream;
}

template <class Message>
typename std::enable_if<std::is_base_of<::google::protobuf::Message, Message>::value, QDataStream&>::type operator>>(
    QDataStream& stream, Message& message_proto) {
  QByteArray message_bytes;
  stream >> message_bytes;
  if (!message_proto.ParseFromArray(message_bytes.data(), message_bytes.size())) throw Parser::ParseError();
  return stream;
}

template <class Message>
Message parsePayload(const QByteArray& payload_bytes) {
  Message message;
  if (!message.ParseFromArray(payload_bytes.data(), payload_bytes.size())) throw Parser::ParseError();
  return message;
}

}  // namespace

QByteArray Parser::genHandshake(const Handshake& message_struct) {
  serialization::Handshake message_protobuf;
  message_protobuf.set_auth_token(convert<std::string>(message_struct.auth_token));
  message_protobuf.set_device_name(convert<std::string>(message_struct.device_name));
  message_protobuf.set_user_agent(convert<std::string>(message_struct.user_agent));
  message_protobuf.set_dht_port((quint16)message_struct.dht_port);

  return serializeMessage(HANDSHAKE, message_protobuf);
}
Handshake Parser::parseHandshake(const QByteArray& payload_bytes) {
  auto message_protobuf = parsePayload<serialization::Handshake>(payload_bytes);

  Handshake message_struct;
  message_struct.auth_token = convert<QByteArray>(message_protobuf.auth_token());
  message_struct.device_name = convert<QString>(message_protobuf.device_name());
  message_struct.user_agent = convert<QString>(message_protobuf.user_agent());
  message_struct.dht_port = (quint16)message_protobuf.dht_port();

  return message_struct;
}

QByteArray Parser::genIndexUpdate(const IndexUpdate& message_struct) {
  serialization::HaveMeta message_protobuf;

  message_protobuf.set_path_keyed_hash(convert<std::string>(message_struct.revision.path_keyed_hash_));
  message_protobuf.set_revision(message_struct.revision.revision_);
  message_protobuf.set_bitfield(convert<std::string>(message_struct.bitfield));

  return serializeMessage(INDEXUPDATE, message_protobuf);
}
IndexUpdate Parser::parseIndexUpdate(const QByteArray& payload_bytes) {
  auto message_protobuf = parsePayload<serialization::HaveMeta>(payload_bytes);

  IndexUpdate message_struct;
  message_struct.revision.revision_ = message_protobuf.revision();
  message_struct.revision.path_keyed_hash_ = convert<QByteArray>(message_protobuf.path_keyed_hash());
  message_struct.bitfield = convert<QBitArray>(message_protobuf.bitfield());

  return message_struct;
}

QByteArray Parser::genMetaRequest(const MetaRequest& message_struct) {
  serialization::MetaRequest message_protobuf;
  message_protobuf.set_path_keyed_hash(convert<std::string>(message_struct.revision.path_keyed_hash_));
  message_protobuf.set_revision(message_struct.revision.revision_);

  return serializeMessage(METAREQUEST, message_protobuf);
}
MetaRequest Parser::parseMetaRequest(const QByteArray& payload_bytes) {
  auto message_protobuf = parsePayload<serialization::MetaRequest>(payload_bytes);

  MetaRequest message_struct;
  message_struct.revision.path_keyed_hash_ = convert<QByteArray>(message_protobuf.path_keyed_hash());
  message_struct.revision.revision_ = message_protobuf.revision();

  return message_struct;
}

QByteArray Parser::genMetaResponse(const MetaResponse& message_struct) {
  serialization::MetaResponse message_protobuf;
  message_protobuf.set_meta(convert<std::string>(message_struct.smeta.rawMetaInfo()));
  message_protobuf.set_signature(convert<std::string>(message_struct.smeta.signature()));
  message_protobuf.set_bitfield(convert<std::string>(message_struct.bitfield));

  return serializeMessage(METARESPONSE, message_protobuf);
}
MetaResponse Parser::parseMetaResponse(const QByteArray& payload_bytes) {
  auto message_protobuf = parsePayload<serialization::MetaResponse>(payload_bytes);

  MetaResponse message_struct;
  message_struct.smeta = {
      convert<QByteArray>(message_protobuf.meta()),
      convert<QByteArray>(message_protobuf.signature()),
  };
  message_struct.bitfield = convert<QBitArray>(message_protobuf.bitfield());

  return message_struct;
}

QByteArray Parser::genBlockRequest(const BlockRequest& message_struct) {
  serialization::BlockRequest message_protobuf;
  message_protobuf.set_ct_hash(convert<std::string>(message_struct.ct_hash));
  message_protobuf.set_offset(message_struct.offset);
  message_protobuf.set_length(message_struct.length);

  return serializeMessage(BLOCKREQUEST, message_protobuf);
}
BlockRequest Parser::parseBlockRequest(const QByteArray& payload_bytes) {
  auto message_protobuf = parsePayload<serialization::BlockRequest>(payload_bytes);

  BlockRequest message_struct;
  message_struct.ct_hash = convert<QByteArray>(message_protobuf.ct_hash());
  message_struct.offset = message_protobuf.offset();
  message_struct.length = message_protobuf.length();

  return message_struct;
}

QByteArray Parser::genBlockResponse(const BlockResponse& message_struct) {
  serialization::BlockResponse message_protobuf;
  message_protobuf.set_ct_hash(convert<std::string>(message_struct.ct_hash));
  message_protobuf.set_offset(message_struct.offset);
  message_protobuf.set_content(convert<std::string>(message_struct.content));

  return serializeMessage(BLOCKRESPONSE, message_protobuf);
}
BlockResponse Parser::parseBlockResponse(const QByteArray& payload_bytes) {
  auto message_protobuf = parsePayload<serialization::BlockResponse>(payload_bytes);

  BlockResponse message_struct;
  message_struct.ct_hash = convert<QByteArray>(message_protobuf.ct_hash());
  message_struct.offset = message_protobuf.offset();
  message_struct.content = convert<QByteArray>(message_protobuf.content());

  return message_struct;
}

std::tuple<Header, QByteArray /*unpacked_payload*/> Parser::parseMessage(const QByteArray& message) {
  QDataStream stream(message);  // naive implementation
  prepareDataStream(stream);

  serialization::Header header_proto;
  QByteArray payload;
  stream >> header_proto >> payload;

  Header header{(MessageType)header_proto.type()};

  return std::make_tuple(header, payload);
}

QByteArray Parser::serializeMessage(MessageType type) {
  serialization::Header header_proto;
  header_proto.set_type(type);

  QByteArray message_bytes;
  QDataStream stream(&message_bytes, QIODevice::Append);
  prepareDataStream(stream);

  stream << header_proto;

  return message_bytes;
}

QByteArray Parser::serializeMessage(MessageType type, const ::google::protobuf::Message& payload_proto) {
  serialization::Header header_proto;
  header_proto.set_type(type);

  QByteArray message_bytes(header_proto.ByteSize(), 0);
  message_bytes.reserve(4 + header_proto.ByteSize() + 4 + payload_proto.ByteSize());
  QDataStream stream(&message_bytes, QIODevice::Append);
  prepareDataStream(stream);

  stream << header_proto << payload_proto;

  return message_bytes;
}

} /* namespace v2 */
} /* namespace protocol */
} /* namespace librevault */
