/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "ProtobufParser.h"
#include "WireProtocol.pb.h"

namespace librevault {

blob ProtobufParser::gen_Handshake(const Handshake& message_struct) {
	protocol::Handshake message_protobuf;
	message_protobuf.set_dir_hash(message_struct.dir_hash.data(), message_struct.dir_hash.size());
	message_protobuf.set_auth_token(message_struct.auth_token.data(), message_struct.auth_token.size());

	return prepare_proto_message(message_protobuf, HANDSHAKE);
}
AbstractParser::Handshake ProtobufParser::parse_Handshake(const blob& message_raw) {
	protocol::Handshake message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	Handshake message_struct;
	message_struct.dir_hash.assign(message_protobuf.dir_hash().begin(), message_protobuf.dir_hash().end());
	message_struct.auth_token.assign(message_protobuf.auth_token().begin(), message_protobuf.auth_token().end());

	return message_struct;
}

blob ProtobufParser::gen_HaveMeta(const HaveMeta& message_struct) {
	protocol::HaveMeta message_protobuf;

	message_protobuf.set_path_id(message_struct.revision.path_id_.data(), message_struct.revision.path_id_.size());
	message_protobuf.set_revision(message_struct.revision.revision_);
	message_protobuf.set_bitfield(message_struct.bitfield.data(), message_struct.bitfield.size());

	return prepare_proto_message(message_protobuf, HAVE_META);
}
AbstractParser::HaveMeta ProtobufParser::parse_HaveMeta(const blob& message_raw) {
	protocol::HaveMeta message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	HaveMeta message_struct;
	message_struct.revision.revision_ = message_protobuf.revision();
	message_struct.revision.path_id_ = blob(message_protobuf.path_id().begin(), message_protobuf.path_id().end());
	message_struct.bitfield = blob(message_protobuf.bitfield().begin(), message_protobuf.bitfield().end());

	return message_struct;
}

blob ProtobufParser::gen_HaveBlock(const HaveBlock& message_struct) {
	protocol::HaveBlock message_protobuf;

	message_protobuf.set_encrypted_data_hash(message_struct.encrypted_data_hash.data(), message_struct.encrypted_data_hash.size());

	return prepare_proto_message(message_protobuf, HAVE_BLOCK);
}
AbstractParser::HaveBlock ProtobufParser::parse_HaveBlock(const blob& message_raw) {
	protocol::HaveBlock message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	HaveBlock message_struct;
	message_struct.encrypted_data_hash = blob(message_protobuf.encrypted_data_hash().begin(), message_protobuf.encrypted_data_hash().end());

	return message_struct;
}

blob ProtobufParser::gen_MetaRequest(const MetaRequest& message_struct) {
	protocol::MetaRequest message_protobuf;
	message_protobuf.set_path_id(message_struct.revision.path_id_.data(), message_struct.revision.path_id_.size());
	message_protobuf.set_revision(message_struct.revision.revision_);

	return prepare_proto_message(message_protobuf, META_REQUEST);
}
AbstractParser::MetaRequest ProtobufParser::parse_MetaRequest(const blob& message_raw) {
	protocol::MetaRequest message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	MetaRequest message_struct;
	message_struct.revision.path_id_.assign(message_protobuf.path_id().begin(), message_protobuf.path_id().end());
	message_struct.revision.revision_ = message_protobuf.revision();

	return message_struct;
}

blob ProtobufParser::gen_MetaReply(const MetaReply& message_struct) {
	protocol::MetaReply message_protobuf;
	message_protobuf.set_meta(message_struct.smeta.meta_.data(), message_struct.smeta.meta_.size());
	message_protobuf.set_signature(message_struct.smeta.signature_.data(), message_struct.smeta.signature_.size());

	return prepare_proto_message(message_protobuf, META_REPLY);
}
AbstractParser::MetaReply ProtobufParser::parse_MetaReply(const blob& message_raw) {
	protocol::MetaReply message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	MetaReply message_struct;
	message_struct.smeta.meta_.assign(message_protobuf.meta().begin(), message_protobuf.meta().end());
	message_struct.smeta.signature_.assign(message_protobuf.signature().begin(), message_protobuf.signature().end());

	return message_struct;
}

blob ProtobufParser::gen_MetaCancel(const MetaCancel& message_struct) {
	protocol::MetaCancel message_protobuf;
	message_protobuf.set_path_id(message_struct.revision.path_id_.data(), message_struct.revision.path_id_.size());
	message_protobuf.set_revision(message_struct.revision.revision_);

	return prepare_proto_message(message_protobuf, META_CANCEL);
}
AbstractParser::MetaCancel ProtobufParser::parse_MetaCancel(const blob& message_raw) {
	protocol::MetaCancel message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	MetaCancel message_struct;
	message_struct.revision.path_id_.assign(message_protobuf.path_id().begin(), message_protobuf.path_id().end());
	message_struct.revision.revision_ = message_protobuf.revision();

	return message_struct;
}

blob ProtobufParser::gen_ChunkRequest(const ChunkRequest& message_struct) {
	protocol::ChunkRequest message_protobuf;
	message_protobuf.set_encrypted_data_hash(message_struct.encrypted_data_hash.data(), message_struct.encrypted_data_hash.size());
	message_protobuf.set_offset(message_struct.offset);
	message_protobuf.set_length(message_struct.length);

	return prepare_proto_message(message_protobuf, CHUNK_REQUEST);
}
AbstractParser::ChunkRequest ProtobufParser::parse_ChunkRequest(const blob& message_raw) {
	protocol::ChunkRequest message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	ChunkRequest message_struct;
	message_struct.encrypted_data_hash.assign(message_protobuf.encrypted_data_hash().begin(), message_protobuf.encrypted_data_hash().end());
	message_struct.offset = message_protobuf.offset();
	message_struct.length = message_protobuf.length();

	return message_struct;
}

blob ProtobufParser::gen_ChunkReply(const ChunkReply& message_struct) {
	protocol::ChunkReply message_protobuf;
	message_protobuf.set_encrypted_data_hash(message_struct.encrypted_data_hash.data(), message_struct.encrypted_data_hash.size());
	message_protobuf.set_offset(message_struct.offset);
	message_protobuf.set_content(message_struct.content.data(), message_struct.content.size());

	return prepare_proto_message(message_protobuf, CHUNK_REPLY);
}
AbstractParser::ChunkReply ProtobufParser::parse_ChunkReply(const blob& message_raw) {
	protocol::ChunkReply message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	ChunkReply message_struct;
	message_struct.encrypted_data_hash.assign(message_protobuf.encrypted_data_hash().begin(), message_protobuf.encrypted_data_hash().end());
	message_struct.offset = message_protobuf.offset();
	message_struct.content.assign(message_protobuf.content().begin(), message_protobuf.content().end());

	return message_struct;
}

blob ProtobufParser::gen_ChunkCancel(const ChunkCancel& message_struct) {
	protocol::ChunkCancel message_protobuf;
	message_protobuf.set_encrypted_data_hash(message_struct.encrypted_data_hash.data(), message_struct.encrypted_data_hash.size());
	message_protobuf.set_offset(message_struct.offset);
	message_protobuf.set_length(message_struct.length);

	return prepare_proto_message(message_protobuf, CHUNK_CANCEL);
}
AbstractParser::ChunkCancel ProtobufParser::parse_ChunkCancel(const blob& message_raw) {
	protocol::ChunkCancel message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	ChunkCancel message_struct;
	message_struct.encrypted_data_hash.assign(message_protobuf.encrypted_data_hash().begin(), message_protobuf.encrypted_data_hash().end());
	message_struct.offset = message_protobuf.offset();
	message_struct.length = message_protobuf.length();

	return message_struct;
}

} /* namespace librevault */
