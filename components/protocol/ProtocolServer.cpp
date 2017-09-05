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
#include "ProtocolServer.h"

namespace librevault {

namespace {

QByteArray wrapProtocol(const ::google::protobuf::Message& payload) {
  protocol::v2::ProtocolMessage message;
  message.mutable_payload()->PackFrom(payload);

  QByteArray buffer(message.ByteSize(), 0);
  message.SerializeToArray(buffer.data(), buffer.size());
  return buffer;
}

}

void ProtocolServer::parse(const QByteArray& message) const {
  protocol::v2::ProtocolMessage message_proto;
  message_proto.ParseFromArray(message, message.size());
  if(message_proto.payload().Is<protocol::v2::Handshake>()) {
    protocol::v2::Handshake payload;
    message_proto.payload().UnpackTo(&payload);
    emit parsedHandshake(QByteArray::fromStdString(payload.auth_token()),
                         QString::fromStdString(payload.device_name()),
                         QString::fromStdString(payload.user_agent()),
                         (quint16)payload.dht_port()
    );
  }else if(message_proto.payload().Is<protocol::v2::IndexUpdate>()) {
    protocol::v2::IndexUpdate payload;
    message_proto.payload().UnpackTo(&payload);
    emit parsedIndexUpdate(QByteArray::fromStdString(payload.path_id()),
                           payload.revision(),
                           QBitArray()
    );
  }else if(message_proto.payload().Is<protocol::v2::MetaRequest>()) {
    protocol::v2::MetaRequest payload;
    message_proto.payload().UnpackTo(&payload);
    emit parsedMetaRequest(QByteArray::fromStdString(payload.path_id()),
                           payload.revision()
    );
  }else if(message_proto.payload().Is<protocol::v2::MetaResponse>()) {
    protocol::v2::MetaResponse payload;
    message_proto.payload().UnpackTo(&payload);
    emit parsedMetaResponse(QByteArray::fromStdString(payload.meta()),
                            QByteArray::fromStdString(payload.signature()),
                            QBitArray()
    );
  }else if(message_proto.payload().Is<protocol::v2::BlockRequest>()) {
    protocol::v2::BlockRequest payload;
    message_proto.payload().UnpackTo(&payload);
    emit parsedBlockRequest(QByteArray::fromStdString(payload.ct_hash()),
                            payload.offset(),
                            payload.length()
    );
  }else if(message_proto.payload().Is<protocol::v2::BlockResponse>()) {
    protocol::v2::BlockResponse payload;
    message_proto.payload().UnpackTo(&payload);
    emit parsedBlockResponse(QByteArray::fromStdString(payload.ct_hash()),
                            payload.offset(),
                            QByteArray::fromStdString(payload.content())
    );
  }
}

void ProtocolServer::serializeHandshake(const QByteArray& auth_token, const QString& device_name, const QString& user_agent, quint16 dht_port) const {
  protocol::v2::Handshake payload;
  payload.set_auth_token(auth_token.toStdString());
  payload.set_device_name(device_name.toStdString());
  payload.set_user_agent(user_agent.toStdString());
  payload.set_dht_port(dht_port);
  emit serialized(wrapProtocol(payload));
}
void ProtocolServer::serializeIndexUpdate(const QByteArray& path_id, quint64 revision, const QBitArray& bitfield) const {
  protocol::v2::IndexUpdate payload;
  payload.set_path_id(path_id.toStdString());
  payload.set_revision(revision);
  payload.set_bitfield("");
  emit serialized(wrapProtocol(payload));
}
void ProtocolServer::serializeMetaRequest(const QByteArray& path_id, quint64 revision) const {
  protocol::v2::MetaRequest payload;
  payload.set_path_id(path_id.toStdString());
  payload.set_revision(revision);
  emit serialized(wrapProtocol(payload));
}
void ProtocolServer::serializeMetaResponse(const QByteArray& meta, const QByteArray& signature, const QBitArray& bitfield) const {
  protocol::v2::MetaResponse payload;
  payload.set_meta(meta.toStdString());
  payload.set_signature(signature.toStdString());
  payload.set_bitfield("");
  emit serialized(wrapProtocol(payload));
}
void ProtocolServer::serializeBlockRequest(const QByteArray& ct_hash, quint64 offset, quint32 length) const {
  protocol::v2::BlockRequest payload;
  payload.set_ct_hash(ct_hash.toStdString());
  payload.set_offset(offset);
  payload.set_length(length);
  emit serialized(wrapProtocol(payload));
}
void ProtocolServer::serializeBlockResponse(const QByteArray& ct_hash, quint64 offset, const QByteArray& content) const {
  protocol::v2::BlockResponse payload;
  payload.set_ct_hash(ct_hash.toStdString());
  payload.set_offset(offset);
  std::string preallocated_block(content.toStdString());
  payload.set_allocated_content(&preallocated_block);
  emit serialized(wrapProtocol(payload));
}

} /* namespace librevault */
