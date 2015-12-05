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
#include "P2PDirectory.h"
#include "P2PProvider.h"
#include "../../Client.h"
#include "../Exchanger.h"
#include "../ExchangeGroup.h"

#include "parsers/ProtobufParser.h"

namespace librevault {

P2PDirectory::P2PDirectory(Client& client, Exchanger& exchanger, P2PProvider& provider, std::string name, websocketpp::connection_hdl connection_handle) :
		RemoteDirectory(client, exchanger),
		provider_(provider),
		connection_handle_(connection_handle) {
	name_ = name;
	role_ = P2PProvider::SERVER;
	log_->trace() << log_tag() << "Created";
	parser_ = std::make_unique<ProtobufParser>();
}

P2PDirectory::P2PDirectory(Client& client, Exchanger &exchanger, P2PProvider &provider, std::string name, websocketpp::connection_hdl connection_handle, std::shared_ptr<ExchangeGroup> exchange_group) :
		P2PDirectory(client, exchanger, provider, name, connection_handle) {
	role_ = P2PProvider::CLIENT;
	exchange_group_ = exchange_group;
}

P2PDirectory::~P2PDirectory() {
	log_->trace() << log_tag() << "Destroyed";
}

tcp_endpoint P2PDirectory::remote_endpoint() const {
	tcp_endpoint endpoint;
	switch(role_){
		case P2PProvider::SERVER: {
			auto con_ptr = provider_.ws_server().get_con_from_hdl(connection_handle_);
			endpoint = con_ptr->get_raw_socket().remote_endpoint();
		} break;
		case P2PProvider::CLIENT: {
			auto con_ptr = provider_.ws_client().get_con_from_hdl(connection_handle_);
			endpoint = con_ptr->get_raw_socket().remote_endpoint();
		} break;
	}
	return endpoint;
}

blob P2PDirectory::local_token() {
	return provider_.node_key().public_key() | crypto::HMAC_SHA3_224(exchange_group_.lock()->key().get_Public_Key());
}

blob P2PDirectory::remote_token() {
	return remote_pubkey() | crypto::HMAC_SHA3_224(exchange_group_.lock()->key().get_Public_Key());
}

void P2PDirectory::send_message(const blob& message) {
	switch(role_){
		case P2PProvider::SERVER: {
			provider_.ws_server().get_con_from_hdl(connection_handle_)->send(message.data(), message.size());
		} break;
		case P2PProvider::CLIENT: {
			provider_.ws_client().get_con_from_hdl(connection_handle_)->send(message.data(), message.size());
		} break;
	}
}

void P2PDirectory::perform_handshake() {
	if(!exchange_group_.lock()) throw protocol_error();

	AbstractParser::Handshake message_struct;
	message_struct.auth_token = local_token();
	message_struct.dir_hash = exchange_group_.lock()->hash();

	send_message(parser_->gen_Handshake(message_struct));
}

/* RPC Actions */
void P2PDirectory::choke() {
	blob message = {AbstractParser::CHOKE};
	send_message(message);
	log_->debug() << log_tag() << "==> CHOKE";
}
void P2PDirectory::unchoke() {
	blob message = {AbstractParser::UNCHOKE};
	send_message(message);
	log_->debug() << log_tag() << "==> UNCHOKE";
}
void P2PDirectory::interest() {
	blob message = {AbstractParser::INTERESTED};
	send_message(message);
	log_->debug() << log_tag() << "==> INTERESTED";
}
void P2PDirectory::uninterest() {
	blob message = {AbstractParser::NOT_INTERESTED};
	send_message(message);
	log_->debug() << log_tag() << "==> NOT_INTERESTED";
}

void P2PDirectory::post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	AbstractParser::HaveMeta message;
	message.revision = revision;
	message.bitfield = convert_bitfield(bitfield);
	send_message(parser_->gen_HaveMeta(message));
}
void P2PDirectory::post_have_block(const blob& encrypted_data_hash) {
	AbstractParser::HaveBlock message;
	message.encrypted_data_hash = encrypted_data_hash;
	send_message(parser_->gen_HaveBlock(message));
}

void P2PDirectory::request_meta(const Meta::PathRevision& revision) {
	AbstractParser::MetaRequest message;
	message.revision = revision;
	send_message(parser_->gen_MetaRequest(message));
}
void P2PDirectory::post_meta(const Meta::SignedMeta& smeta) {
	AbstractParser::MetaReply message;
	message.smeta = smeta;
	send_message(parser_->gen_MetaReply(message));
}
void P2PDirectory::cancel_meta(const Meta::PathRevision& revision) {
	AbstractParser::MetaCancel message;
	message.revision = revision;
	send_message(parser_->gen_MetaCancel(message));
}

void P2PDirectory::request_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t length) {
	AbstractParser::ChunkRequest message;
	message.encrypted_data_hash = encrypted_data_hash;
	message.offset = offset;
	message.length = length;
	send_message(parser_->gen_ChunkRequest(message));
}
void P2PDirectory::post_chunk(const blob& encrypted_data_hash, uint32_t offset, const blob& chunk) {
	AbstractParser::ChunkReply message;
	message.encrypted_data_hash = encrypted_data_hash;
	message.offset = offset;
	message.content = chunk;
	send_message(parser_->gen_ChunkReply(message));
}
void P2PDirectory::cancel_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t length) {
	AbstractParser::ChunkCancel message;
	message.encrypted_data_hash = encrypted_data_hash;
	message.offset = offset;
	message.length = length;
	send_message(parser_->gen_ChunkCancel(message));
}
/*
void P2PDirectory::disconnect(const boost::system::error_code& error) {
	auto this_shared = shared_from_this();	// To prevent deletion during execution of this function. After exiting from the scope it will do "delete this;" internally
	if(disconnect_mtx_.try_lock()){
		connection_->disconnect(error);

		log_->debug() << "Connection to " << connection_->remote_string() << " closed: " << error.message();
		// Detach from ExchangeGroup
		auto group_ptr = exchange_group_.lock();
		if(group_ptr) group_ptr->detach(shared_from_this());

		// Remove from temporary provider's pool
		provider_.remove_from_unattached(this_shared);
	}
}*/

void P2PDirectory::handle_message(const blob& message_raw) {
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

void P2PDirectory::handle_Handshake(const blob& message_raw) {
	auto message_struct = parser_->parse_Handshake(message_raw);

	// Attaching to ExchangeGroup
	auto group_ptr = exchanger_.get_group(message_struct.dir_hash);
	if(group_ptr)
		group_ptr->attach(shared_from_this());
	else throw ExchangeGroup::attach_error();

	// Checking authentication using token
	if(message_struct.auth_token != remote_token()) throw auth_error();

	if(role_ == P2PProvider::SERVER) perform_handshake();

	log_->debug() << log_tag() << "LV Handshake successful";
	handshake_performed_ = true;
}

void P2PDirectory::handle_Choke(const blob& message_raw) {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_Unchoke(const blob& message_raw) {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_Interested(const blob& message_raw) {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_NotInterested(const blob& message_raw) {
#   warning "Not implemented yet"
}

void P2PDirectory::handle_HaveMeta(const blob& message_raw) {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_HaveBlock(const blob& message_raw) {
#   warning "Not implemented yet"
}

void P2PDirectory::handle_MetaRequest(const blob& message_raw) {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_MetaReply(const blob& message_raw) {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_MetaCancel(const blob& message_raw) {
#   warning "Not implemented yet"
}

void P2PDirectory::handle_ChunkRequest(const blob& message_raw) {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_ChunkReply(const blob& message_raw) {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_ChunkCancel(const blob& message_raw) {
#   warning "Not implemented yet"
}

/*
void P2PDirectory::post_revision(std::shared_ptr<AbstractDirectory> origin, const Meta::PathRevision& revision) {
	AbstractParser::MetaList meta_list;
	meta_list.revision.push_back(revision);

	log_->debug() << log_tag() << "Posting revision " << revision.revision_ << " of Meta " << path_id_readable(revision.path_id_);
	connection_->send(parser_->gen_meta_list(meta_list), []{});
}

void P2PDirectory::request_meta(std::shared_ptr<AbstractDirectory> origin, const blob& path_id) {
	AbstractParser::MetaRequest meta_request;
	meta_request.path_id = path_id;

	log_->debug() << log_tag() << "Requesting Meta " << path_id_readable(path_id);
	connection_->send(parser_->gen_meta_request(meta_request), []{});
}

void P2PDirectory::post_meta(std::shared_ptr<AbstractDirectory> origin, const Meta::SignedMeta& smeta) {
	AbstractParser::Meta meta;
	meta.smeta = smeta;

	log_->debug() << log_tag() << "Posting Meta";
	connection_->send(parser_->gen_meta(meta), []{});
}

void P2PDirectory::request_block(std::shared_ptr<AbstractDirectory> origin, const blob& block_id) {
	AbstractParser::BlockRequest block_request;
	block_request.block_id = block_id;

	log_->debug() << log_tag() << "Requesting block " << encrypted_data_hash_readable(block_id);
	connection_->send(parser_->gen_block_request(block_request), []{});
}

void P2PDirectory::post_block(std::shared_ptr<AbstractDirectory> origin, const blob& block_id, const blob& block) {
	AbstractParser::Block block_message;
	block_message.block_id = block_id;
	block_message.block_content = block;

	log_->debug() << log_tag() << "Posting block " << encrypted_data_hash_readable(block_id);
	connection_->send(parser_->gen_block(block_message), []{});
}*/

void P2PDirectory::send_meta_list() {
#   warning "Not implemented yet"
}

} /* namespace librevault */
