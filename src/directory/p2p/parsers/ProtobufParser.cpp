/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ProtobufParser.h"
#include "WireProtocol.pb.h"

namespace librevault {

blob ProtobufParser::gen_handshake(const Handshake& message_struct) {
	protocol::Handshake message_protobuf;
	message_protobuf.set_user_agent(message_struct.user_agent);
	message_protobuf.set_dir_hash(message_struct.dir_hash.data(), message_struct.dir_hash.size());
	message_protobuf.set_auth_token(message_struct.auth_token.data(), message_struct.auth_token.size());

	return prepare_proto_message(message_protobuf, HANDSHAKE);
}

AbstractParser::Handshake ProtobufParser::parse_handshake(const blob& message_raw) {
	protocol::Handshake message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw P2PDirectory::protocol_error();

	Handshake message_struct;
	message_struct.user_agent = message_protobuf.user_agent();
	message_struct.dir_hash.assign(message_protobuf.dir_hash().begin(), message_protobuf.dir_hash().end());
	message_struct.auth_token.assign(message_protobuf.auth_token().begin(), message_protobuf.auth_token().end());

	return message_struct;
}

blob ProtobufParser::gen_meta_list(const MetaList& message_struct) {
	protocol::MetaList message_protobuf;
	for(auto one_revision : message_struct.revision) {
		auto list_ptr = message_protobuf.add_list();
		list_ptr->set_path_id(one_revision.path_id_.data(), one_revision.path_id_.size());
		list_ptr->set_revision(one_revision.revision_);
	}

	return prepare_proto_message(message_protobuf, META_LIST);
}

AbstractParser::MetaList ProtobufParser::parse_meta_list(const blob& message_raw) {
	protocol::MetaList message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw P2PDirectory::protocol_error();

	MetaList message_struct;
	for(auto one_revision : message_protobuf.list()) {
		message_struct.revision.push_back({
												  blob(one_revision.path_id().begin(), one_revision.path_id().end()),
												  one_revision.revision()
										  });
	}

	return message_struct;
}

blob ProtobufParser::gen_meta_request(const MetaRequest& message_struct) {
	protocol::MetaRequest message_protobuf;
	message_protobuf.set_path_id(message_struct.path_id.data(), message_struct.path_id.size());

	return prepare_proto_message(message_protobuf, META_REQUEST);
}

AbstractParser::MetaRequest ProtobufParser::parse_meta_request(const blob& message_raw) {
	protocol::MetaRequest message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw P2PDirectory::protocol_error();

	MetaRequest message_struct;
	message_struct.path_id.assign(message_protobuf.path_id().begin(), message_protobuf.path_id().end());

	return message_struct;
}

blob ProtobufParser::gen_meta(const Meta& message_struct) {
	protocol::Meta message_protobuf;
	message_protobuf.set_meta(message_struct.smeta.meta.data(), message_struct.smeta.meta.size());
	message_protobuf.set_signature(message_struct.smeta.signature.data(), message_struct.smeta.signature.size());

	return prepare_proto_message(message_protobuf, META);
}

AbstractParser::Meta ProtobufParser::parse_meta(const blob& message_raw) {
	protocol::Meta message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw P2PDirectory::protocol_error();

	Meta message_struct;
	message_struct.smeta.meta.assign(message_protobuf.meta().begin(), message_protobuf.meta().end());
	message_struct.smeta.signature.assign(message_protobuf.signature().begin(), message_protobuf.signature().end());

	return message_struct;
}

blob ProtobufParser::gen_block_request(const BlockRequest& message_struct) {
	protocol::BlockRequest message_protobuf;
	message_protobuf.set_block_id(message_struct.block_id.data(), message_struct.block_id.size());

	return prepare_proto_message(message_protobuf, BLOCK_REQUEST);
}

ProtobufParser::BlockRequest ProtobufParser::parse_block_request(const blob& message_raw) {
	protocol::BlockRequest message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw P2PDirectory::protocol_error();

	BlockRequest message_struct;
	message_struct.block_id.assign(message_protobuf.block_id().begin(), message_protobuf.block_id().end());

	return message_struct;
}

blob ProtobufParser::gen_block(const Block& message_struct) {
	protocol::Block message_protobuf;
	message_protobuf.set_block_id(message_struct.block_id.data(), message_struct.block_id.size());
	message_protobuf.set_block_content(message_struct.block_content.data(), message_struct.block_content.size());

	return prepare_proto_message(message_protobuf, BLOCK);
}

ProtobufParser::Block ProtobufParser::parse_block(const blob& message_raw) {
	protocol::Block message_protobuf;
	if(!message_protobuf.ParseFromArray(message_raw.data()+1, message_raw.size()-1)) throw P2PDirectory::protocol_error();

	Block message_struct;
	message_struct.block_id.assign(message_protobuf.block_id().begin(), message_protobuf.block_id().end());
	message_struct.block_content.assign(message_protobuf.block_content().begin(), message_protobuf.block_content().end());

	return message_struct;
}

} /* namespace librevault */
