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
#include "V2Protocol.pb.h"
#include "ProtocolParser.h"
#include <QLoggingCategory>

namespace librevault {

Q_LOGGING_CATEGORY(log_protocol_parsing, "protocol.parsing");

namespace {

template <class Message>
Message parsePayload(const std::string& payload_binary) {
  Message payload_proto;
  if(! payload_proto.ParseFromString(payload_binary)) {
    throw ProtocolParser::ParserError(
            std::string("Could not parse protobuf payload of type ") + payload_proto.descriptor()->name()
    );
  }
  return payload_proto;
}

template <class Message>
QByteArray serializeAsByteArray(Message message_proto) {
  QByteArray message_bytes(message_proto.ByteSize(), 0);
  if(!message_proto.SerializeToArray(message_bytes.data(), message_bytes.size())) {
    throw ProtocolParser::ParserError(
            std::string("Could not serialize protobuf message of type ") + message_proto.descriptor()->name()
    );
  }
  return message_bytes;
}

}

QVariantMap ProtocolParser::parse(const QByteArray& message_bytes) const {
  protocol::v2::ProtocolMessage message_proto;
  message_proto.ParseFromArray(message_bytes, message_bytes.size());

  QVariantMap message;
  message["type"] = QString::fromStdString(protocol::v2::MessageType_Name(message_proto.type()));
  switch (message_proto.type()) {
    case protocol::v2::HANDSHAKE: {
      auto payload_proto = parsePayload<protocol::v2::Handshake>(message_proto.payload());
      message["auth_token"] = QByteArray::fromStdString(payload_proto.auth_token());
      message["device_name"] = QString::fromStdString(payload_proto.device_name());
      message["user_agent"] = QString::fromStdString(payload_proto.user_agent());
      message["dht_port"] = payload_proto.dht_port();
    } break;
    case protocol::v2::CHOKE: break;
    case protocol::v2::UNCHOKE: break;
    case protocol::v2::INTERESTED: break;
    case protocol::v2::NOTINTERESTED: break;
    case protocol::v2::INDEXUPDATE: {
      auto payload_proto = parsePayload<protocol::v2::IndexUpdate>(message_proto.payload());
      message["path_id"] = QByteArray::fromStdString(payload_proto.path_id());
      message["revision"] = (quint64)payload_proto.revision();
      message["bitfield"] = QByteArray::fromStdString(payload_proto.bitfield());
    } break;
    case protocol::v2::METAREQUEST: {
      auto payload_proto = parsePayload<protocol::v2::MetaRequest>(message_proto.payload());
      message["path_id"] = QByteArray::fromStdString(payload_proto.path_id());
      message["revision"] = (quint64)payload_proto.revision();
    } break;
    case protocol::v2::METARESPONSE: {
      auto payload_proto = parsePayload<protocol::v2::MetaResponse>(message_proto.payload());
      message["meta"] = QByteArray::fromStdString(payload_proto.meta());
      message["signature"] = QByteArray::fromStdString(payload_proto.signature());
      message["bitfield"] = QByteArray::fromStdString(payload_proto.bitfield());
    } break;
    case protocol::v2::BLOCKREQUEST: {
      auto payload_proto = parsePayload<protocol::v2::BlockRequest>(message_proto.payload());
      message["ct_hash"] = QByteArray::fromStdString(payload_proto.ct_hash());
      message["offset"] = (quint64)payload_proto.offset();
      message["length"] = (quint32)payload_proto.length();
    } break;
    case protocol::v2::BLOCKRESPONSE: {
      auto payload_proto = parsePayload<protocol::v2::BlockResponse>(message_proto.payload());
      message["ct_hash"] = QByteArray::fromStdString(payload_proto.ct_hash());
      message["offset"] = (quint64)payload_proto.offset();
      message["content"] = QByteArray::fromStdString(payload_proto.content());
    } break;
    default: throw ParserError("Invalid message type in Protobuf message");
  }

  qCDebug(log_protocol_parsing) << "Parsed message" << "Protobuf:" << message_proto.DebugString() << "Qt:" << message;
  return message;
}

QByteArray ProtocolParser::serialize(const QVariantMap& message) const {
  protocol::v2::ProtocolMessage message_proto;

  protocol::v2::MessageType payload_type;
  if(! protocol::v2::MessageType_Parse(message["type"].toString().toStdString(), &payload_type)) {
    throw ParserError("Invalid message type in QVariantMap");
  }
  message_proto.set_type(payload_type);

  std::string payload_bytes_buffer;
  switch (payload_type) {
    case protocol::v2::HANDSHAKE: {
      protocol::v2::Handshake payload_proto;
      payload_proto.set_auth_token(message["auth_token"].toByteArray().toStdString());
      payload_proto.set_device_name(message["device_name"].toString().toStdString());
      payload_proto.set_user_agent(message["user_agent"].toString().toStdString());
      payload_proto.set_dht_port(message["user_agent"].toUInt());
      payload_bytes_buffer = payload_proto.SerializeAsString();
    } break;
    case protocol::v2::CHOKE: break;
    case protocol::v2::UNCHOKE: break;
    case protocol::v2::INTERESTED: break;
    case protocol::v2::NOTINTERESTED: break;
    case protocol::v2::INDEXUPDATE: {
      protocol::v2::IndexUpdate payload_proto;
      payload_proto.set_path_id(message["path_id"].toByteArray().toStdString());
      payload_proto.set_revision(message["revision"].toUInt());
      payload_proto.set_bitfield(message["bitfield"].toByteArray().toStdString());  // todo
      payload_bytes_buffer = payload_proto.SerializeAsString();
    } break;
    case protocol::v2::METAREQUEST: {
      protocol::v2::MetaRequest payload_proto;
      payload_proto.set_path_id(message["path_id"].toByteArray().toStdString());
      payload_proto.set_revision(message["revision"].toULongLong());
      payload_bytes_buffer = payload_proto.SerializeAsString();
    } break;
    case protocol::v2::METARESPONSE: {
      protocol::v2::MetaResponse payload_proto;
      payload_proto.set_meta(message["meta"].toByteArray().toStdString());
      payload_proto.set_signature(message["signature"].toByteArray().toStdString());
      payload_proto.set_bitfield(message["bitfield"].toByteArray().toStdString());
      payload_bytes_buffer = payload_proto.SerializeAsString();
    } break;
    case protocol::v2::BLOCKREQUEST: {
      protocol::v2::BlockRequest payload_proto;
      payload_proto.set_ct_hash(message["ct_hash"].toByteArray().toStdString());
      payload_proto.set_offset(message["offset"].toULongLong());
      payload_proto.set_length(message["length"].toUInt());
      payload_bytes_buffer = payload_proto.SerializeAsString();
    } break;
    case protocol::v2::BLOCKRESPONSE: {
      protocol::v2::BlockResponse payload_proto;
      payload_proto.set_ct_hash(message["ct_hash"].toByteArray().toStdString());
      payload_proto.set_offset(message["offset"].toULongLong());
      payload_proto.set_content(message["content"].toByteArray().toStdString());
      payload_bytes_buffer = payload_proto.SerializeAsString();
    } break;
    default: throw ParserError("Invalid message type in QVariantMap message");
  }

  message_proto.set_allocated_payload(&payload_bytes_buffer);

  qCDebug(log_protocol_parsing) << "Serialized message" << "Qt:" << message << "Protobuf:" << message_proto.DebugString();
  return serializeAsByteArray(message_proto);
}

} /* namespace librevault */
