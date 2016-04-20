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
#include "include/librevault/protocol/V1Parser.h"
#include "V1Protocol.pb.h"

namespace librevault {

blob V1Parser::gen_Handshake(const Handshake& message_struct) {
	protocol::Handshake message_protobuf;
	message_protobuf.set_auth_token(message_struct.auth_token.data(), message_struct.auth_token.size());
	message_protobuf.set_device_name(message_struct.device_name.data(), message_struct.device_name.size());
	message_protobuf.set_user_agent(message_struct.user_agent.data(), message_struct.user_agent.size());

	return prepare_proto_message(message_protobuf, HANDSHAKE);
}
V1Parser::Handshake V1Parser::parse_Handshake(const blob& message_raw) {
	protocol::Handshake message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	Handshake message_struct;
	message_struct.auth_token.assign(message_protobuf.auth_token().begin(), message_protobuf.auth_token().end());
	message_struct.device_name.assign(message_protobuf.device_name().begin(), message_protobuf.device_name().end());
	message_struct.user_agent.assign(message_protobuf.user_agent().begin(), message_protobuf.user_agent().end());

	return message_struct;
}

blob V1Parser::gen_HaveMeta(const HaveMeta& message_struct) {
	protocol::HaveMeta message_protobuf;

	message_protobuf.set_path_id(message_struct.revision.path_id_.data(), message_struct.revision.path_id_.size());
	message_protobuf.set_revision(message_struct.revision.revision_);

	blob converted_bitfield = convert_bitfield(message_struct.bitfield);
	message_protobuf.set_bitfield(converted_bitfield.data(), converted_bitfield.size());

	return prepare_proto_message(message_protobuf, HAVE_META);
}
V1Parser::HaveMeta V1Parser::parse_HaveMeta(const blob& message_raw) {
	protocol::HaveMeta message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	HaveMeta message_struct;
	message_struct.revision.revision_ = message_protobuf.revision();
	message_struct.revision.path_id_ = blob(message_protobuf.path_id().begin(), message_protobuf.path_id().end());
	message_struct.bitfield = convert_bitfield(blob(message_protobuf.bitfield().begin(), message_protobuf.bitfield().end()));

	return message_struct;
}

blob V1Parser::gen_HaveChunk(const HaveChunk& message_struct) {
	protocol::HaveChunk message_protobuf;

	message_protobuf.set_ct_hash(message_struct.ct_hash.data(), message_struct.ct_hash.size());

	return prepare_proto_message(message_protobuf, HAVE_CHUNK);
}
V1Parser::HaveChunk V1Parser::parse_HaveChunk(const blob& message_raw) {
	protocol::HaveChunk message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	HaveChunk message_struct;
	message_struct.ct_hash = blob(message_protobuf.ct_hash().begin(), message_protobuf.ct_hash().end());

	return message_struct;
}

blob V1Parser::gen_MetaRequest(const MetaRequest& message_struct) {
	protocol::MetaRequest message_protobuf;
	message_protobuf.set_path_id(message_struct.revision.path_id_.data(), message_struct.revision.path_id_.size());
	message_protobuf.set_revision(message_struct.revision.revision_);

	return prepare_proto_message(message_protobuf, META_REQUEST);
}
V1Parser::MetaRequest V1Parser::parse_MetaRequest(const blob& message_raw) {
	protocol::MetaRequest message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	MetaRequest message_struct;
	message_struct.revision.path_id_.assign(message_protobuf.path_id().begin(), message_protobuf.path_id().end());
	message_struct.revision.revision_ = message_protobuf.revision();

	return message_struct;
}

blob V1Parser::gen_MetaReply(const MetaReply& message_struct) {
	protocol::MetaReply message_protobuf;
	message_protobuf.set_meta(message_struct.smeta.raw_meta().data(), message_struct.smeta.raw_meta().size());
	message_protobuf.set_signature(message_struct.smeta.signature().data(), message_struct.smeta.signature().size());

	blob converted_bitfield = convert_bitfield(message_struct.bitfield);
	message_protobuf.set_bitfield(converted_bitfield.data(), converted_bitfield.size());

	return prepare_proto_message(message_protobuf, META_REPLY);
}
V1Parser::MetaReply V1Parser::parse_MetaReply(const blob& message_raw, const Secret& secret_verifier) {
	protocol::MetaReply message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	blob raw_meta(message_protobuf.meta().begin(), message_protobuf.meta().end());
	blob signature(message_protobuf.signature().begin(), message_protobuf.signature().end());

	bitfield_type converted_bitfield = convert_bitfield(blob(message_protobuf.bitfield().begin(), message_protobuf.bitfield().end()));

	return MetaReply{SignedMeta(std::move(raw_meta), std::move(signature), secret_verifier), std::move(converted_bitfield)};
}

blob V1Parser::gen_MetaCancel(const MetaCancel& message_struct) {
	protocol::MetaCancel message_protobuf;
	message_protobuf.set_path_id(message_struct.revision.path_id_.data(), message_struct.revision.path_id_.size());
	message_protobuf.set_revision(message_struct.revision.revision_);

	return prepare_proto_message(message_protobuf, META_CANCEL);
}
V1Parser::MetaCancel V1Parser::parse_MetaCancel(const blob& message_raw) {
	protocol::MetaCancel message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	MetaCancel message_struct;
	message_struct.revision.path_id_.assign(message_protobuf.path_id().begin(), message_protobuf.path_id().end());
	message_struct.revision.revision_ = message_protobuf.revision();

	return message_struct;
}

blob V1Parser::gen_BlockRequest(const BlockRequest& message_struct) {
	protocol::BlockRequest message_protobuf;
	message_protobuf.set_ct_hash(message_struct.ct_hash.data(), message_struct.ct_hash.size());
	message_protobuf.set_offset(message_struct.offset);
	message_protobuf.set_length(message_struct.length);

	return prepare_proto_message(message_protobuf, BLOCK_REQUEST);
}
V1Parser::BlockRequest V1Parser::parse_BlockRequest(const blob& message_raw) {
	protocol::BlockRequest message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	BlockRequest message_struct;
	message_struct.ct_hash.assign(message_protobuf.ct_hash().begin(), message_protobuf.ct_hash().end());
	message_struct.offset = message_protobuf.offset();
	message_struct.length = message_protobuf.length();

	return message_struct;
}

blob V1Parser::gen_BlockReply(const BlockReply& message_struct) {
	protocol::BlockReply message_protobuf;
	message_protobuf.set_ct_hash(message_struct.ct_hash.data(), message_struct.ct_hash.size());
	message_protobuf.set_offset(message_struct.offset);
	message_protobuf.set_content(message_struct.content.data(), message_struct.content.size());

	return prepare_proto_message(message_protobuf, BLOCK_REPLY);
}
V1Parser::BlockReply V1Parser::parse_BlockReply(const blob& message_raw) {
	protocol::BlockReply message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	BlockReply message_struct;
	message_struct.ct_hash.assign(message_protobuf.ct_hash().begin(), message_protobuf.ct_hash().end());
	message_struct.offset = message_protobuf.offset();
	message_struct.content.assign(message_protobuf.content().begin(), message_protobuf.content().end());

	return message_struct;
}

blob V1Parser::gen_BlockCancel(const BlockCancel& message_struct) {
	protocol::BlockCancel message_protobuf;
	message_protobuf.set_ct_hash(message_struct.ct_hash.data(), message_struct.ct_hash.size());
	message_protobuf.set_offset(message_struct.offset);
	message_protobuf.set_length(message_struct.length);

	return prepare_proto_message(message_protobuf, BLOCK_CANCEL);
}
V1Parser::BlockCancel V1Parser::parse_BlockCancel(const blob& message_raw) {
	protocol::BlockCancel message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw parse_error();

	BlockCancel message_struct;
	message_struct.ct_hash.assign(message_protobuf.ct_hash().begin(), message_protobuf.ct_hash().end());
	message_struct.offset = message_protobuf.offset();
	message_struct.length = message_protobuf.length();

	return message_struct;
}

} /* namespace librevault */
