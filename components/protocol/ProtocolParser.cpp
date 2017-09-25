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
#include "ProtocolParser.h"
#include "V2Protocol.pb.h"
#include <conv_bitfield.h>
#include <QDataStream>
#include <QLoggingCategory>

namespace librevault {
namespace protocol {
namespace v2 {

Q_LOGGING_CATEGORY(log_protocol_parsing, "protocol.parsing");

namespace {

template <class Message>
Message parseProtobuf(const QByteArray& bytes) {
  Message proto;
  if (!proto.ParseFromArray(bytes, bytes.size())) {
    throw ProtocolParser::ParserError(std::string("Could not parse protobuf message of type ") +
                                      proto.descriptor()->name());
  }
  return proto;
}

template <class Message>
QByteArray serializeProtobuf(const Message& message_proto) {
  QByteArray message_bytes(message_proto.ByteSize(), 0);
  if (!message_proto.SerializeToArray(message_bytes.data(), message_bytes.size())) {
    throw ProtocolParser::ParserError(std::string("Could not serialize protobuf message of type ") +
                                      message_proto.descriptor()->name());
  }
  return message_bytes;
}

void logMessage(const QVariantMap& message_qt, const ::google::protobuf::Message& header,
                const ::google::protobuf::Message& payload) {
  qCDebug(log_protocol_parsing) << "Message conversion;"
                                << "Qt:" << message_qt
                                << "Protobuf header:" << QString::fromStdString(header.DebugString())
                                << "Protobuf payload:" << QString::fromStdString(payload.DebugString());
}

void logMessage(const QVariantMap& message_qt, const ::google::protobuf::Message& header) {
  qCDebug(log_protocol_parsing) << "Message conversion;"
                                << "Qt:" << message_qt
                                << "Protobuf header:" << QString::fromStdString(header.DebugString());
}

}  // namespace

QVariantMap ProtocolParser::parse(const QByteArray& message_bytes) const {
  QByteArray header_bytes, payload_bytes;
  QDataStream stream(message_bytes);
  stream.setByteOrder(QDataStream::BigEndian);
  stream.setVersion(QDataStream::Qt_5_0);
  stream >> header_bytes;
  stream >> payload_bytes;

  Header header_proto = parseProtobuf<Header>(header_bytes);

  QVariantMap message;
  message["type"] = QString::fromStdString(MessageType_Name(header_proto.type())).toLower();
  switch (header_proto.type()) {
    case HANDSHAKE: {
      auto payload_proto = parseProtobuf<Handshake>(payload_bytes);
      message["auth_token"] = QByteArray::fromStdString(payload_proto.auth_token());
      message["device_name"] = QString::fromStdString(payload_proto.device_name());
      message["user_agent"] = QString::fromStdString(payload_proto.user_agent());
      message["dht_port"] = payload_proto.dht_port();
      logMessage(message, header_proto, payload_proto);
    } break;
    case CHOKE:
    case UNCHOKE:
    case INTERESTED:
    case NOTINTERESTED:
      logMessage(message, header_proto);
      break;
    case INDEXUPDATE: {
      auto payload_proto = parseProtobuf<IndexUpdate>(payload_bytes);
      message["path_keyed_hash"] = QByteArray::fromStdString(payload_proto.path_keyed_hash());
      message["revision"] = (quint64)payload_proto.revision();
      message["bitfield"] = QByteArray::fromStdString(payload_proto.bitfield());
      logMessage(message, header_proto, payload_proto);
    } break;
    case METAREQUEST: {
      auto payload_proto = parseProtobuf<MetaRequest>(payload_bytes);
      message["path_keyed_hash"] = QByteArray::fromStdString(payload_proto.path_keyed_hash());
      message["revision"] = (quint64)payload_proto.revision();
      logMessage(message, header_proto, payload_proto);
    } break;
    case METARESPONSE: {
      auto payload_proto = parseProtobuf<MetaResponse>(payload_bytes);
      message["metainfo"] = QByteArray::fromStdString(payload_proto.metainfo());
      message["signature"] = QByteArray::fromStdString(payload_proto.signature());
      message["bitfield"] = convert_bitfield(QByteArray::fromStdString(payload_proto.bitfield()));
      logMessage(message, header_proto, payload_proto);
    } break;
    case BLOCKREQUEST: {
      auto payload_proto = parseProtobuf<BlockRequest>(payload_bytes);
      message["ct_hash"] = QByteArray::fromStdString(payload_proto.ct_hash());
      message["offset"] = (quint64)payload_proto.offset();
      message["length"] = (quint32)payload_proto.length();
      logMessage(message, header_proto, payload_proto);
    } break;
    case BLOCKRESPONSE: {
      auto payload_proto = parseProtobuf<BlockResponse>(payload_bytes);
      message["ct_hash"] = QByteArray::fromStdString(payload_proto.ct_hash());
      message["offset"] = (quint64)payload_proto.offset();
      message["content"] = QByteArray::fromStdString(payload_proto.content());
      logMessage(message, header_proto, payload_proto);
    } break;
    default:
      throw ParserError("Invalid message type in Protobuf message");
  }

  return message;
}

QByteArray ProtocolParser::serialize(const QVariantMap& message) const {
  Header header_proto;

  MessageType payload_type;
  if (!MessageType_Parse(message["type"].toString().toUpper().toStdString(), &payload_type)) {
    throw ParserError("Invalid message type in QVariantMap");
  }
  header_proto.set_type(payload_type);
  QByteArray header_bytes = serializeProtobuf(header_proto);

  QByteArray payload_bytes;
  switch (payload_type) {
    case HANDSHAKE: {
      Handshake payload_proto;
      payload_proto.set_auth_token(message["auth_token"].toByteArray().toStdString());
      payload_proto.set_device_name(message["device_name"].toString().toStdString());
      payload_proto.set_user_agent(message["user_agent"].toString().toStdString());
      payload_proto.set_dht_port(message["dht_port"].toUInt());
      payload_bytes = serializeProtobuf(payload_proto);
      logMessage(message, header_proto, payload_proto);
    } break;
    case CHOKE:
    case UNCHOKE:
    case INTERESTED:
    case NOTINTERESTED:
      logMessage(message, header_proto);
      break;
    case INDEXUPDATE: {
      IndexUpdate payload_proto;
      payload_proto.set_path_keyed_hash(message["path_keyed_hash"].toByteArray().toStdString());
      payload_proto.set_revision(message["revision"].toUInt());
      payload_proto.set_bitfield(convert_bitfield(message["bitfield"].toBitArray()).toStdString());
      payload_bytes = serializeProtobuf(payload_proto);
      logMessage(message, header_proto, payload_proto);
    } break;
    case METAREQUEST: {
      MetaRequest payload_proto;
      payload_proto.set_path_keyed_hash(message["path_keyed_hash"].toByteArray().toStdString());
      payload_proto.set_revision(message["revision"].toULongLong());
      payload_bytes = serializeProtobuf(payload_proto);
      logMessage(message, header_proto, payload_proto);
    } break;
    case METARESPONSE: {
      MetaResponse payload_proto;
      payload_proto.set_metainfo(message["metainfo"].toByteArray().toStdString());
      payload_proto.set_signature(message["signature"].toByteArray().toStdString());
      payload_proto.set_bitfield(convert_bitfield(message["bitfield"].toBitArray()).toStdString());
      payload_bytes = serializeProtobuf(payload_proto);
      logMessage(message, header_proto, payload_proto);
    } break;
    case BLOCKREQUEST: {
      BlockRequest payload_proto;
      payload_proto.set_ct_hash(message["ct_hash"].toByteArray().toStdString());
      payload_proto.set_offset(message["offset"].toULongLong());
      payload_proto.set_length(message["length"].toUInt());
      payload_bytes = serializeProtobuf(payload_proto);
      logMessage(message, header_proto, payload_proto);
    } break;
    case BLOCKRESPONSE: {
      BlockResponse payload_proto;
      payload_proto.set_ct_hash(message["ct_hash"].toByteArray().toStdString());
      payload_proto.set_offset(message["offset"].toULongLong());
      payload_proto.set_content(message["content"].toByteArray().toStdString());
      payload_bytes = serializeProtobuf(payload_proto);
      logMessage(message, header_proto, payload_proto);
    } break;
    default:
      throw ParserError("Invalid message type in QVariantMap message");
  }

  QByteArray result;
  result.reserve(4 + header_bytes.size() + 4 + payload_bytes.size());
  QDataStream stream(&result, QIODevice::Append);
  stream.setByteOrder(QDataStream::BigEndian);
  stream.setVersion(QDataStream::Qt_5_0);
  stream << header_bytes;
  stream << payload_bytes;

  return result;
}

}  // namespace v2
}  // namespace protocol
} /* namespace librevault */
