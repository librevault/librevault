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

template <class ProtoMessage>
typename std::enable_if<std::is_base_of<::google::protobuf::Message, ProtoMessage>::value,
    QDataStream&>::type
operator>>(QDataStream& stream, ProtoMessage& message_proto) {
  QByteArray message_bytes;
  stream >> message_bytes;
  if (!message_proto.ParseFromArray(message_bytes, message_bytes.size()))
    throw Parser::ParseError();
  return stream;
}

template <class ProtoMessage>
ProtoMessage parsePayload(const QByteArray& payload_bytes) {
  ProtoMessage message;
  if (!message.ParseFromArray(payload_bytes, payload_bytes.size())) throw Parser::ParseError();
  return message;
}

}  // namespace

QByteArray Parser::serialize(const Message& msg) const {
  QByteArray result;
  QDataStream stream(&result, QIODevice::Append);
  prepareDataStream(stream);

  serialization::Header header;
  header.set_type((quint32)msg.header.type);
  stream << header;

  switch (msg.header.type) {
    case Message::Header::MessageType::HANDSHAKE: {
      serialization::Handshake payload;
      payload.set_auth_token(convert<std::string>(msg.handshake.auth_token));
      payload.set_device_name(convert<std::string>(msg.handshake.device_name));
      payload.set_user_agent(convert<std::string>(msg.handshake.user_agent));
      payload.set_dht_port((quint16)msg.handshake.dht_port);
      stream << payload;
    }
    case Message::Header::MessageType::INDEXUPDATE: {
      serialization::IndexUpdate payload;
      payload.set_path_keyed_hash(convert<std::string>(msg.indexupdate.revision.path_keyed_hash_));
      payload.set_revision(msg.indexupdate.revision.revision_);
      payload.set_bitfield(convert<std::string>(msg.indexupdate.bitfield));
      stream << payload;
    }
    case Message::Header::MessageType::METAREQUEST: {
      serialization::MetaRequest payload;
      payload.set_path_keyed_hash(convert<std::string>(msg.metarequest.revision.path_keyed_hash_));
      payload.set_revision(msg.metarequest.revision.revision_);
      stream << payload;
    }
    case Message::Header::MessageType::METARESPONSE: {
      serialization::MetaResponse payload;
      payload.set_meta(convert<std::string>(msg.metaresponse.smeta.rawMetaInfo()));
      payload.set_signature(convert<std::string>(msg.metaresponse.smeta.signature()));
      payload.set_bitfield(convert<std::string>(msg.metaresponse.bitfield));
      stream << payload;
    }
    case Message::Header::MessageType::BLOCKREQUEST: {
      serialization::BlockRequest payload;
      payload.set_ct_hash(convert<std::string>(msg.blockrequest.ct_hash));
      payload.set_offset(msg.blockrequest.offset);
      payload.set_length(msg.blockrequest.length);
      stream << payload;
    }
    case Message::Header::MessageType::BLOCKRESPONSE: {
      serialization::BlockResponse payload;
      payload.set_ct_hash(convert<std::string>(msg.blockresponse.ct_hash));
      payload.set_offset(msg.blockresponse.offset);
      payload.set_content(convert<std::string>(msg.blockresponse.content));
      stream << payload;
    }
    default:;
  }
  return result;
}

Message Parser::parse(const QByteArray& binary) const {
  QDataStream stream(binary);  // naive implementation
  prepareDataStream(stream);

  serialization::Header header_proto;
  QByteArray payload_bytes;
  stream >> header_proto >> payload_bytes;

  Message result;
  result.header.type = (Message::Header::MessageType)header_proto.type();

  switch (result.header.type) {
    case Message::Header::MessageType::HANDSHAKE: {
      auto message_protobuf = parsePayload<serialization::Handshake>(payload_bytes);
      result.handshake.auth_token = convert<QByteArray>(message_protobuf.auth_token());
      result.handshake.device_name = convert<QString>(message_protobuf.device_name());
      result.handshake.user_agent = convert<QString>(message_protobuf.user_agent());
      result.handshake.dht_port = (quint16)message_protobuf.dht_port();
    }
    case Message::Header::MessageType::INDEXUPDATE: {
      auto message_protobuf = parsePayload<serialization::IndexUpdate>(payload_bytes);
      result.indexupdate.revision.revision_ = message_protobuf.revision();
      result.indexupdate.revision.path_keyed_hash_ =
          convert<QByteArray>(message_protobuf.path_keyed_hash());
      result.indexupdate.bitfield = convert<QBitArray>(message_protobuf.bitfield());
    }
    case Message::Header::MessageType::METAREQUEST: {
      auto message_protobuf = parsePayload<serialization::MetaRequest>(payload_bytes);
      result.metarequest.revision.path_keyed_hash_ =
          convert<QByteArray>(message_protobuf.path_keyed_hash());
      result.metarequest.revision.revision_ = message_protobuf.revision();
    }
    case Message::Header::MessageType::METARESPONSE: {
      auto message_protobuf = parsePayload<serialization::MetaResponse>(payload_bytes);
      result.metaresponse.smeta = {
          convert<QByteArray>(message_protobuf.meta()),
          convert<QByteArray>(message_protobuf.signature()),
      };
      result.metaresponse.bitfield = convert<QBitArray>(message_protobuf.bitfield());
    }
    case Message::Header::MessageType::BLOCKREQUEST: {
      auto message_protobuf = parsePayload<serialization::BlockRequest>(payload_bytes);
      result.blockrequest.ct_hash = convert<QByteArray>(message_protobuf.ct_hash());
      result.blockrequest.offset = message_protobuf.offset();
      result.blockrequest.length = message_protobuf.length();
    }
    case Message::Header::MessageType::BLOCKRESPONSE: {
      auto message_protobuf = parsePayload<serialization::BlockResponse>(payload_bytes);
      result.blockresponse.ct_hash = convert<QByteArray>(message_protobuf.ct_hash());
      result.blockresponse.offset = message_protobuf.offset();
      result.blockresponse.content = convert<QByteArray>(message_protobuf.content());
    }
    default:;
  }

  return result;
}

} /* namespace v2 */
} /* namespace protocol */
} /* namespace librevault */
