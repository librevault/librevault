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
#include "ProtocolServer.h"
#include "V2Protocol.pb.h"
#include <conv_bitfield.h>
#include <QDataStream>

namespace librevault {
namespace protocol {
namespace v2 {

namespace {

template <class ProtoMessageClass>
ProtoMessageClass parseProtobuf(const QByteArray& payload_bytes) {
  ProtoMessageClass payload_proto;
  if (payload_bytes.isEmpty() || !payload_proto.ParseFromArray(payload_bytes.data(), payload_bytes.size()))
    throw librevault::protocol::v2::ProtocolServer::ParseError();
  return payload_proto;
}

template <class ProtoMessageClass>
QByteArray serializeMessage(librevault::protocol::v2::MessageType type, const ProtoMessageClass& payload_proto) {
  Header header_proto;
  header_proto.set_type(type);

  QByteArray result;
  result.reserve(4 + header_proto.ByteSize() + 4 + payload_proto.ByteSize());

  QDataStream stream(&result, QIODevice::Append);
  stream.setByteOrder(QDataStream::BigEndian);
  stream.setVersion(QDataStream::Qt_5_0);

  stream << QByteArray::fromStdString(header_proto.SerializeAsString());
  stream << QByteArray::fromStdString(payload_proto.SerializeAsString());

  return result;
}

QByteArray serializeHeader(librevault::protocol::v2::MessageType type) {
  Header header_proto;
  header_proto.set_type(type);

  QByteArray result;
  result.reserve(4 + header_proto.ByteSize());

  QDataStream stream(&result, QIODevice::Append);
  stream.setByteOrder(QDataStream::BigEndian);
  stream.setVersion(QDataStream::Qt_5_0);

  stream << QByteArray::fromStdString(header_proto.SerializeAsString());

  return result;
}

}  // namespace

void ProtocolServer::parse(const QByteArray& message_bytes) {
  try {
    QDataStream stream(message_bytes);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setVersion(QDataStream::Qt_5_0);

    QByteArray header_bytes, payload_bytes;
    stream >> header_bytes;
    stream >> payload_bytes;

    if (header_bytes.isEmpty()) throw MessageTruncated();

    Header header = parseProtobuf<Header>(header_bytes);
    switch (header.type()) {
      case HANDSHAKE: {
        auto payload = parseProtobuf<Handshake>(payload_bytes);
        parsedHandshake(QByteArray::fromStdString(payload.auth_token()), QString::fromStdString(payload.device_name()),
                        QString::fromStdString(payload.user_agent()), (quint16)payload.dht_port());
      } break;
      case CHOKE:
        parsedChoke();
      case UNCHOKE:
        parsedUnchoke();
      case INTERESTED:
        parsedInterested();
      case NOTINTERESTED:
        parsedNotInterested();
      case INDEXUPDATE: {
        auto payload = parseProtobuf<IndexUpdate>(payload_bytes);
        parsedIndexUpdate({QByteArray::fromStdString(payload.path_keyed_hash()), payload.revision()},
                          convert_bitfield(QByteArray::fromStdString(payload.bitfield())));
      } break;
      case METAREQUEST: {
        auto payload = parseProtobuf<MetaRequest>(payload_bytes);
        parsedMetaRequest({QByteArray::fromStdString(payload.path_keyed_hash()), payload.revision()});
      } break;
      case METARESPONSE: {
        auto payload = parseProtobuf<MetaResponse>(payload_bytes);
        parsedMetaResponse(
            SignedMeta(QByteArray::fromStdString(payload.metainfo()), QByteArray::fromStdString(payload.signature())),
            convert_bitfield(QByteArray::fromStdString(payload.bitfield())));
      } break;
      case BLOCKREQUEST: {
        auto payload = parseProtobuf<BlockRequest>(payload_bytes);
        parsedBlockRequest(QByteArray::fromStdString(payload.ct_hash()), payload.offset(), payload.length());
      } break;
      case BLOCKRESPONSE: {
        auto payload = parseProtobuf<BlockResponse>(payload_bytes);
        parsedBlockResponse(QByteArray::fromStdString(payload.ct_hash()), payload.offset(),
                            QByteArray::fromStdString(payload.content()));
      } break;
      default:
        throw ParseError();
    }
  } catch (const std::exception& e) {
    emit parseFailed(e.what(), message_bytes);
  }
}

void ProtocolServer::serializeHandshake(const QByteArray& auth_token, const QString& device_name,
                                        const QString& user_agent, quint16 dht_port) {
  Handshake payload;
  payload.set_auth_token(auth_token.toStdString());
  payload.set_device_name(device_name.toStdString());
  payload.set_user_agent(user_agent.toStdString());
  payload.set_dht_port(dht_port);
  emit serialized(serializeMessage(HANDSHAKE, payload));
}

void ProtocolServer::serializeChoke() { emit serialized(serializeHeader(CHOKE)); }
void ProtocolServer::serializeUnchoke() { emit serialized(serializeHeader(UNCHOKE)); }
void ProtocolServer::serializeInterested() { emit serialized(serializeHeader(INTERESTED)); }
void ProtocolServer::serializeNotInterested() { emit serialized(serializeHeader(NOTINTERESTED)); }

void ProtocolServer::serializeIndexUpdate(const MetaInfo::PathRevision& revision, const QBitArray& bitfield) {
  IndexUpdate payload;
  payload.set_path_keyed_hash(revision.path_keyed_hash_.toStdString());
  payload.set_revision(revision.revision_);
  payload.set_bitfield(convert_bitfield(bitfield).toStdString());
  emit serialized(serializeMessage(INDEXUPDATE, payload));
}

void ProtocolServer::serializeMetaRequest(const MetaInfo::PathRevision& revision) {
  MetaRequest payload;
  payload.set_path_keyed_hash(revision.path_keyed_hash_.toStdString());
  payload.set_revision(revision.revision_);
  emit serialized(serializeMessage(METAREQUEST, payload));
}
void ProtocolServer::serializeMetaResponse(const SignedMeta& smeta, const QBitArray& bitfield) {
  MetaResponse payload;
  payload.set_metainfo(smeta.rawMetaInfo().toStdString());
  payload.set_signature(smeta.signature().toStdString());
  payload.set_bitfield(convert_bitfield(bitfield).toStdString());
  emit serialized(serializeMessage(METARESPONSE, payload));
}

void ProtocolServer::serializeBlockRequest(const QByteArray& ct_hash, quint64 offset, quint32 length) {
  BlockRequest payload;
  payload.set_ct_hash(ct_hash.toStdString());
  payload.set_offset(offset);
  payload.set_length(length);
  emit serialized(serializeMessage(BLOCKREQUEST, payload));
}
void ProtocolServer::serializeBlockResponse(const QByteArray& ct_hash, quint64 offset, const QByteArray& content) {
  BlockResponse payload;
  payload.set_ct_hash(ct_hash.toStdString());
  payload.set_offset(offset);
  payload.set_content(content.data(), content.size());
  emit serialized(serializeMessage(BLOCKRESPONSE, payload));
}

}  // namespace v2
}  // namespace protocol
}  // namespace librevault
