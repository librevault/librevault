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
#include "P2PFolder.h"
#include "src/Client.h"
#include "src/folder/FolderGroup.h"

#include <librevault/protocol/ProtobufParser.h>

namespace librevault {

P2PFolder::P2PFolder(Client& client, P2PProvider& provider, std::string name, websocketpp::connection_hdl connection_handle, P2PProvider::role_type role) :
	RemoteFolder(client),
	provider_(provider),
	connection_handle_(connection_handle) {
	name_ = name;
	role_ = role;
	log_->trace() << log_tag() << "Created";
	parser_ = std::make_unique<ProtobufParser>();
}

P2PFolder::P2PFolder(Client& client, P2PProvider& provider, std::string name, websocketpp::connection_hdl connection_handle) :
	P2PFolder(client, provider, name, connection_handle, P2PProvider::SERVER) {}

P2PFolder::P2PFolder(Client& client, P2PProvider& provider, std::string name, websocketpp::connection_hdl connection_handle, std::shared_ptr<FolderGroup> folder_group) :
	P2PFolder(client, provider, name, connection_handle, P2PProvider::CLIENT) {
	folder_group_ = folder_group;
}

P2PFolder::~P2PFolder() {
	log_->debug() << log_tag() << "Destroyed";
}

void P2PFolder::update_remote_endpoint() {
	switch(role_){
		case P2PProvider::SERVER: {
			auto con_ptr = provider_.ws_server().get_con_from_hdl(connection_handle_);
			remote_endpoint_ = con_ptr->get_raw_socket().remote_endpoint();
		} break;
		case P2PProvider::CLIENT: {
			auto con_ptr = provider_.ws_client().get_con_from_hdl(connection_handle_);
			remote_endpoint_ = con_ptr->get_raw_socket().remote_endpoint();
		} break;
	}
}

blob P2PFolder::local_token() {
	return provider_.node_key().public_key() | crypto::HMAC_SHA3_224(folder_group_.lock()->secret().get_Public_Key());
}

blob P2PFolder::remote_token() {
	return remote_pubkey() | crypto::HMAC_SHA3_224(folder_group_.lock()->secret().get_Public_Key());
}

void P2PFolder::send_message(const blob& message) {
	switch(role_){
		case P2PProvider::SERVER: {
			provider_.ws_server().get_con_from_hdl(connection_handle_)->send(message.data(), message.size());
		} break;
		case P2PProvider::CLIENT: {
			provider_.ws_client().get_con_from_hdl(connection_handle_)->send(message.data(), message.size());
		} break;
	}
}

void P2PFolder::perform_handshake() {
	if(!exchange_group()) throw protocol_error();

	AbstractParser::Handshake message_struct;
	message_struct.auth_token = local_token();
	message_struct.device_name = client_.config().getDevice_name();

	send_message(parser_->gen_Handshake(message_struct));
	log_->debug() << log_tag() << "==> HANDSHAKE";
}

/* RPC Actions */
void P2PFolder::choke() {
	if(! am_choking_) {
		blob message = {AbstractParser::CHOKE};
		send_message(message);
		am_choking_ = true;

		log_->debug() << log_tag() << "==> CHOKE";
	}
}
void P2PFolder::unchoke() {
	if(am_choking_) {
		blob message = {AbstractParser::UNCHOKE};
		send_message(message);
		am_choking_ = false;

		log_->debug() << log_tag() << "==> UNCHOKE";
	}
}
void P2PFolder::interest() {
	if(! am_interested_) {
		blob message = {AbstractParser::INTERESTED};
		send_message(message);
		am_interested_ = true;

		log_->debug() << log_tag() << "==> INTERESTED";
	}
}
void P2PFolder::uninterest() {
	if(am_interested_) {
		blob message = {AbstractParser::NOT_INTERESTED};
		send_message(message);
		am_interested_ = false;

		log_->debug() << log_tag() << "==> NOT_INTERESTED";
	}
}

void P2PFolder::post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	AbstractParser::HaveMeta message;
	message.revision = revision;
	message.bitfield = bitfield;
	send_message(parser_->gen_HaveMeta(message));

	log_->debug() << log_tag() << "==> HAVE_META:"
		<< " path_id=" << path_id_readable(message.revision.path_id_)
		<< " revision=" << message.revision.revision_
		<< " bits=" << message.bitfield;
}
void P2PFolder::post_have_block(const blob& encrypted_data_hash) {
	AbstractParser::HaveBlock message;
	message.encrypted_data_hash = encrypted_data_hash;
	send_message(parser_->gen_HaveBlock(message));

	log_->debug() << log_tag() << "==> HAVE_BLOCK:"
		<< " encrypted_data_hash=" << encrypted_data_hash_readable(encrypted_data_hash);
}

void P2PFolder::request_meta(const Meta::PathRevision& revision) {
	AbstractParser::MetaRequest message;
	message.revision = revision;
	send_message(parser_->gen_MetaRequest(message));

	log_->debug() << log_tag() << "==> META_REQUEST:"
		<< " path_id=" << path_id_readable(revision.path_id_)
		<< " revision=" << revision.revision_;
}
void P2PFolder::post_meta(const Meta::SignedMeta& smeta, const bitfield_type& bitfield) {
	AbstractParser::MetaReply message;
	message.smeta = smeta;
	message.bitfield = bitfield;
	send_message(parser_->gen_MetaReply(message));

	log_->debug() << log_tag() << "==> META_REPLY:"
		<< " path_id=" << path_id_readable(smeta.meta().path_id())
		<< " revision=" << smeta.meta().revision()
		<< " bits=" << bitfield;
}
void P2PFolder::cancel_meta(const Meta::PathRevision& revision) {
	AbstractParser::MetaCancel message;
	message.revision = revision;
	send_message(parser_->gen_MetaCancel(message));

	log_->debug() << log_tag() << "==> META_CANCEL:"
		<< " path_id=" << path_id_readable(revision.path_id_)
		<< " revision=" << revision.revision_;
}

void P2PFolder::request_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t length) {
	AbstractParser::ChunkRequest message;
	message.encrypted_data_hash = encrypted_data_hash;
	message.offset = offset;
	message.length = length;
	send_message(parser_->gen_ChunkRequest(message));

	log_->debug() << log_tag() << "==> CHUNK_REQUEST:"
		<< " encrypted_data_hash=" << encrypted_data_hash_readable(encrypted_data_hash)
		<< " offset=" << offset
		<< " length=" << length;
}
void P2PFolder::post_chunk(const blob& encrypted_data_hash, uint32_t offset, const blob& chunk) {
	AbstractParser::ChunkReply message;
	message.encrypted_data_hash = encrypted_data_hash;
	message.offset = offset;
	message.content = chunk;
	send_message(parser_->gen_ChunkReply(message));

	log_->debug() << log_tag() << "==> CHUNK_REPLY:"
		<< " encrypted_data_hash=" << encrypted_data_hash_readable(encrypted_data_hash)
		<< " offset=" << offset;
}
void P2PFolder::cancel_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t length) {
	AbstractParser::ChunkCancel message;
	message.encrypted_data_hash = encrypted_data_hash;
	message.offset = offset;
	message.length = length;
	send_message(parser_->gen_ChunkCancel(message));
	log_->debug() << log_tag() << "==> CHUNK_CANCEL:"
		<< " encrypted_data_hash=" << encrypted_data_hash_readable(encrypted_data_hash)
		<< " offset=" << offset
		<< " length=" << length;
}

void P2PFolder::handle_message(const blob& message_raw) {
	AbstractParser::message_type message_type = (AbstractParser::message_type)message_raw[0];

	if(is_handshaken()) {
		switch(message_type) {
			case AbstractParser::CHOKE: handle_Choke(message_raw); break;
			case AbstractParser::UNCHOKE: handle_Unchoke(message_raw); break;
			case AbstractParser::INTERESTED: handle_Interested(message_raw); break;
			case AbstractParser::NOT_INTERESTED: handle_NotInterested(message_raw); break;
			case AbstractParser::HAVE_META: handle_HaveMeta(message_raw); break;
			case AbstractParser::HAVE_BLOCK: handle_HaveBlock(message_raw); break;
			case AbstractParser::META_REQUEST: handle_MetaRequest(message_raw); break;
			case AbstractParser::META_REPLY: handle_MetaReply(message_raw); break;
			case AbstractParser::META_CANCEL: handle_MetaCancel(message_raw); break;
			case AbstractParser::CHUNK_REQUEST: handle_ChunkRequest(message_raw); break;
			case AbstractParser::CHUNK_REPLY: handle_ChunkReply(message_raw); break;
			case AbstractParser::CHUNK_CANCEL: handle_ChunkCancel(message_raw); break;
			default: throw protocol_error();
		}
	}else{
		handle_Handshake(message_raw);
	}
}

void P2PFolder::handle_Handshake(const blob& message_raw) {
	log_->trace() << log_tag() << "handle_Handshake()";
	auto message_struct = parser_->parse_Handshake(message_raw);
	log_->debug() << log_tag() << "<== HANDSHAKE";

	// Attaching to FolderGroup
	auto group_ptr = folder_group_.lock();
	if(folder_group_.lock()) {
		update_remote_endpoint();
		group_ptr->attach(shared_from_this());
	}
	else throw FolderGroup::attach_error();

	// Checking authentication using token
	if(message_struct.auth_token != remote_token()) throw auth_error();

	if(role_ == P2PProvider::SERVER) perform_handshake();

	log_->debug() << log_tag() << "LV Handshake successful";
	is_handshaken_ = true;

	folder_group_.lock()->handle_handshake(shared_from_this());
}

void P2PFolder::handle_Choke(const blob& message_raw) {
	log_->trace() << log_tag() << "handle_Choke()";
	log_->debug() << log_tag() << "<== CHOKE";

	if(! peer_choking_) {
		peer_choking_ = true;
		exchange_group()->handle_choke(shared_from_this());
	}
}
void P2PFolder::handle_Unchoke(const blob& message_raw) {
	log_->trace() << log_tag() << "handle_Unchoke()";
	log_->debug() << log_tag() << "<== UNCHOKE";

	if(peer_choking_) {
		peer_choking_ = false;
		exchange_group()->handle_unchoke(shared_from_this());
	}
}
void P2PFolder::handle_Interested(const blob& message_raw) {
	log_->trace() << log_tag() << "handle_Interested()";
	log_->debug() << log_tag() << "<== INTERESTED";

	if(! peer_interested_) {
		peer_interested_ = true;
		exchange_group()->handle_interested(shared_from_this());
	}
}
void P2PFolder::handle_NotInterested(const blob& message_raw) {
	log_->trace() << log_tag() << "handle_NotInterested()";
	log_->debug() << log_tag() << "<== NOT_INTERESTED";

	if(peer_interested_) {
		peer_interested_ = false;
		exchange_group()->handle_not_interested(shared_from_this());
	}
}

void P2PFolder::handle_HaveMeta(const blob& message_raw) {
	log_->trace() << log_tag() << "handle_HaveMeta()";

	auto message_struct = parser_->parse_HaveMeta(message_raw);
	log_->debug() << log_tag() << "<== HAVE_META:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_
		<< " bits=" << message_struct.bitfield;

	exchange_group()->notify_meta(shared_from_this(), message_struct.revision, message_struct.bitfield);
}
void P2PFolder::handle_HaveBlock(const blob& message_raw) {
	log_->trace() << log_tag() << "handle_HaveBlock()";

	auto message_struct = parser_->parse_HaveBlock(message_raw);
	log_->debug() << log_tag() << "<== HAVE_BLOCK:"
		<< " encrypted_data_hash=" << encrypted_data_hash_readable(message_struct.encrypted_data_hash);
	exchange_group()->notify_block(shared_from_this(), message_struct.encrypted_data_hash);
}

void P2PFolder::handle_MetaRequest(const blob& message_raw) {
	log_->trace() << log_tag() << "handle_MetaRequest()";

	auto message_struct = parser_->parse_MetaRequest(message_raw);
	log_->debug() << log_tag() << "<== META_REQUEST:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_;

	exchange_group()->request_meta(shared_from_this(), message_struct.revision);
}
void P2PFolder::handle_MetaReply(const blob& message_raw) {
	log_->trace() << log_tag() << "handle_MetaReply()";

	auto message_struct = parser_->parse_MetaReply(message_raw, exchange_group()->secret());
	log_->debug() << log_tag() << "<== META_REPLY:"
		<< " path_id=" << path_id_readable(message_struct.smeta.meta().path_id())
		<< " revision=" << message_struct.smeta.meta().revision()
		<< " bits=" << message_struct.bitfield;

	exchange_group()->post_meta(shared_from_this(), message_struct.smeta, message_struct.bitfield);
}
void P2PFolder::handle_MetaCancel(const blob& message_raw) {
#   warning "Not implemented yet"
	log_->trace() << log_tag() << "handle_MetaCancel()";

	auto message_struct = parser_->parse_MetaCancel(message_raw);
	log_->debug() << log_tag() << "<== META_CANCEL:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_;
}

void P2PFolder::handle_ChunkRequest(const blob& message_raw) {
	log_->trace() << log_tag() << "handle_ChunkRequest()";

	auto message_struct = parser_->parse_ChunkRequest(message_raw);
	log_->debug() << log_tag() << "<== CHUNK_REQUEST:"
		<< " encrypted_data_hash=" << encrypted_data_hash_readable(message_struct.encrypted_data_hash)
		<< " length=" << message_struct.length
		<< " offset=" << message_struct.offset;

	exchange_group()->request_chunk(shared_from_this(), message_struct.encrypted_data_hash, message_struct.offset, message_struct.length);
}
void P2PFolder::handle_ChunkReply(const blob& message_raw) {
	log_->trace() << log_tag() << "handle_ChunkReply()";

	auto message_struct = parser_->parse_ChunkReply(message_raw);
	log_->debug() << log_tag() << "<== CHUNK_REPLY:"
		<< " encrypted_data_hash=" << encrypted_data_hash_readable(message_struct.encrypted_data_hash)
		<< " offset=" << message_struct.offset;

	exchange_group()->post_chunk(shared_from_this(), message_struct.encrypted_data_hash, message_struct.content, message_struct.offset);
}
void P2PFolder::handle_ChunkCancel(const blob& message_raw) {
#   warning "Not implemented yet"
	log_->trace() << log_tag() << "handle_ChunkCancel()";

	auto message_struct = parser_->parse_ChunkCancel(message_raw);
	log_->debug() << log_tag() << "<== CHUNK_CANCEL:"
		<< " encrypted_data_hash=" << encrypted_data_hash_readable(message_struct.encrypted_data_hash)
		<< " length=" << message_struct.length
		<< " offset=" << message_struct.offset;
}

} /* namespace librevault */
