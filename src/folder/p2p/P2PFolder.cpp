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
#include "WSService.h"
#include "src/Client.h"
#include "src/control/Config.h"
#include "src/folder/FolderGroup.h"

#include <librevault/Tokens.h>

namespace librevault {

P2PFolder::P2PFolder(Client& client, P2PProvider& provider, WSService& ws_service, std::string name, websocketpp::connection_hdl connection_handle, P2PProvider::role_type role) :
	RemoteFolder(client),
	provider_(provider),
	ws_service_(ws_service),
	connection_handle_(connection_handle) {
	name_ = name;
	role_ = role;
	log_->trace() << log_tag() << "Created";
}

P2PFolder::P2PFolder(Client& client, P2PProvider& provider, WSService& ws_service, std::string name, websocketpp::connection_hdl connection_handle) :
	P2PFolder(client, provider, ws_service, name, connection_handle, P2PProvider::SERVER) {}

P2PFolder::P2PFolder(Client& client, P2PProvider& provider, WSService& ws_service, std::string name, websocketpp::connection_hdl connection_handle, std::shared_ptr<FolderGroup> folder_group) :
	P2PFolder(client, provider, ws_service, name, connection_handle, P2PProvider::CLIENT) {
	folder_group_ = folder_group;
}

P2PFolder::~P2PFolder() {
	log_->debug() << log_tag() << "Destroyed";
}

blob P2PFolder::local_token() {
	return derive_token(folder_group_.lock()->secret(), provider_.node_key().public_key());
}

blob P2PFolder::remote_token() {
	return derive_token(folder_group_.lock()->secret(), remote_pubkey());
}

void P2PFolder::send_message(const blob& message) {
	ws_service_.send_message(connection_handle_, message);
}

void P2PFolder::perform_handshake() {
	if(!folder_group()) throw protocol_error();

	V1Parser::Handshake message_struct;
	message_struct.auth_token = local_token();
	message_struct.device_name = Config::get()->client()["client_name"].asString();

	send_message(parser_.gen_Handshake(message_struct));
	log_->debug() << log_tag() << "==> HANDSHAKE";
}

/* RPC Actions */
void P2PFolder::choke() {
	if(! am_choking_) {
		send_message(parser_.gen_Choke());
		am_choking_ = true;

		log_->debug() << log_tag() << "==> CHOKE";
	}
}
void P2PFolder::unchoke() {
	if(am_choking_) {
		send_message(parser_.gen_Unchoke());
		am_choking_ = false;

		log_->debug() << log_tag() << "==> UNCHOKE";
	}
}
void P2PFolder::interest() {
	if(! am_interested_) {
		send_message(parser_.gen_Interested());
		am_interested_ = true;

		log_->debug() << log_tag() << "==> INTERESTED";
	}
}
void P2PFolder::uninterest() {
	if(am_interested_) {
		send_message(parser_.gen_NotInterested());
		am_interested_ = false;

		log_->debug() << log_tag() << "==> NOT_INTERESTED";
	}
}

void P2PFolder::post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	V1Parser::HaveMeta message;
	message.revision = revision;
	message.bitfield = bitfield;
	send_message(parser_.gen_HaveMeta(message));

	log_->debug() << log_tag() << "==> HAVE_META:"
		<< " path_id=" << path_id_readable(message.revision.path_id_)
		<< " revision=" << message.revision.revision_
		<< " bits=" << message.bitfield;
}
void P2PFolder::post_have_chunk(const blob& ct_hash) {
	V1Parser::HaveChunk message;
	message.ct_hash = ct_hash;
	send_message(parser_.gen_HaveChunk(message));

	log_->debug() << log_tag() << "==> HAVE_BLOCK:"
		<< " ct_hash=" << ct_hash_readable(ct_hash);
}

void P2PFolder::request_meta(const Meta::PathRevision& revision) {
	V1Parser::MetaRequest message;
	message.revision = revision;
	send_message(parser_.gen_MetaRequest(message));

	log_->debug() << log_tag() << "==> META_REQUEST:"
		<< " path_id=" << path_id_readable(revision.path_id_)
		<< " revision=" << revision.revision_;
}
void P2PFolder::post_meta(const SignedMeta& smeta, const bitfield_type& bitfield) {
	V1Parser::MetaReply message;
	message.smeta = smeta;
	message.bitfield = bitfield;
	send_message(parser_.gen_MetaReply(message));

	log_->debug() << log_tag() << "==> META_REPLY:"
		<< " path_id=" << path_id_readable(smeta.meta().path_id())
		<< " revision=" << smeta.meta().revision()
		<< " bits=" << bitfield;
}
void P2PFolder::cancel_meta(const Meta::PathRevision& revision) {
	V1Parser::MetaCancel message;
	message.revision = revision;
	send_message(parser_.gen_MetaCancel(message));

	log_->debug() << log_tag() << "==> META_CANCEL:"
		<< " path_id=" << path_id_readable(revision.path_id_)
		<< " revision=" << revision.revision_;
}

void P2PFolder::request_block(const blob& ct_hash, uint32_t offset, uint32_t length) {
	V1Parser::BlockRequest message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.length = length;
	send_message(parser_.gen_BlockRequest(message));

	log_->debug() << log_tag() << "==> CHUNK_REQUEST:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset
		<< " length=" << length;
}
void P2PFolder::post_block(const blob& ct_hash, uint32_t offset, const blob& chunk) {
	V1Parser::BlockReply message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.content = chunk;
	send_message(parser_.gen_BlockReply(message));

	log_->debug() << log_tag() << "==> CHUNK_REPLY:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset;
}
void P2PFolder::cancel_block(const blob& ct_hash, uint32_t offset, uint32_t length) {
	V1Parser::BlockCancel message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.length = length;
	send_message(parser_.gen_BlockCancel(message));
	log_->debug() << log_tag() << "==> CHUNK_CANCEL:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset
		<< " length=" << length;
}

void P2PFolder::handle_message(const blob& message_raw) {
	V1Parser::message_type message_type = parser_.parse_MessageType(message_raw);

	if(ready()) {
		switch(message_type) {
			case V1Parser::CHOKE: handle_Choke(message_raw); break;
			case V1Parser::UNCHOKE: handle_Unchoke(message_raw); break;
			case V1Parser::INTERESTED: handle_Interested(message_raw); break;
			case V1Parser::NOT_INTERESTED: handle_NotInterested(message_raw); break;
			case V1Parser::HAVE_META: handle_HaveMeta(message_raw); break;
			case V1Parser::HAVE_CHUNK: handle_HaveChunk(message_raw); break;
			case V1Parser::META_REQUEST: handle_MetaRequest(message_raw); break;
			case V1Parser::META_REPLY: handle_MetaReply(message_raw); break;
			case V1Parser::META_CANCEL: handle_MetaCancel(message_raw); break;
			case V1Parser::BLOCK_REQUEST: handle_BlockRequest(message_raw); break;
			case V1Parser::BLOCK_REPLY: handle_BlockReply(message_raw); break;
			case V1Parser::BLOCK_CANCEL: handle_BlockCancel(message_raw); break;
			default: throw protocol_error();
		}
	}else{
		handle_Handshake(message_raw);
	}
}

void P2PFolder::handle_Handshake(const blob& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	auto message_struct = parser_.parse_Handshake(message_raw);
	log_->debug() << log_tag() << "<== HANDSHAKE";

	// Attaching to FolderGroup
	auto group_ptr = folder_group_.lock();
	if(group_ptr)
		group_ptr->attach(shared_from_this());
	else throw FolderGroup::attach_error();

	// Checking authentication using token
	if(message_struct.auth_token != remote_token()) throw auth_error();

	if(role_ == P2PProvider::SERVER) perform_handshake();

	log_->debug() << log_tag() << "LV Handshake successful";
	is_handshaken_ = true;

	group_ptr->handle_handshake(shared_from_this());
}

void P2PFolder::handle_Choke(const blob& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	log_->debug() << log_tag() << "<== CHOKE";

	if(! peer_choking_) {
		peer_choking_ = true;
		folder_group()->handle_choke(shared_from_this());
	}
}
void P2PFolder::handle_Unchoke(const blob& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	log_->debug() << log_tag() << "<== UNCHOKE";

	if(peer_choking_) {
		peer_choking_ = false;
		folder_group()->handle_unchoke(shared_from_this());
	}
}
void P2PFolder::handle_Interested(const blob& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	log_->debug() << log_tag() << "<== INTERESTED";

	if(! peer_interested_) {
		peer_interested_ = true;
		folder_group()->handle_interested(shared_from_this());
	}
}
void P2PFolder::handle_NotInterested(const blob& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	log_->debug() << log_tag() << "<== NOT_INTERESTED";

	if(peer_interested_) {
		peer_interested_ = false;
		folder_group()->handle_not_interested(shared_from_this());
	}
}

void P2PFolder::handle_HaveMeta(const blob& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto message_struct = parser_.parse_HaveMeta(message_raw);
	log_->debug() << log_tag() << "<== HAVE_META:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_
		<< " bits=" << message_struct.bitfield;

	folder_group()->notify_meta(shared_from_this(), message_struct.revision, message_struct.bitfield);
}
void P2PFolder::handle_HaveChunk(const blob& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto message_struct = parser_.parse_HaveChunk(message_raw);
	log_->debug() << log_tag() << "<== HAVE_BLOCK:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash);
	folder_group()->notify_chunk(shared_from_this(), message_struct.ct_hash);
}

void P2PFolder::handle_MetaRequest(const blob& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto message_struct = parser_.parse_MetaRequest(message_raw);
	log_->debug() << log_tag() << "<== META_REQUEST:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_;

	folder_group()->request_meta(shared_from_this(), message_struct.revision);
}
void P2PFolder::handle_MetaReply(const blob& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto message_struct = parser_.parse_MetaReply(message_raw, folder_group()->secret());
	log_->debug() << log_tag() << "<== META_REPLY:"
		<< " path_id=" << path_id_readable(message_struct.smeta.meta().path_id())
		<< " revision=" << message_struct.smeta.meta().revision()
		<< " bits=" << message_struct.bitfield;

	folder_group()->post_meta(shared_from_this(), message_struct.smeta, message_struct.bitfield);
}
void P2PFolder::handle_MetaCancel(const blob& message_raw) {
#   warning "Not implemented yet"
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto message_struct = parser_.parse_MetaCancel(message_raw);
	log_->debug() << log_tag() << "<== META_CANCEL:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_;
}

void P2PFolder::handle_BlockRequest(const blob& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto message_struct = parser_.parse_BlockRequest(message_raw);
	log_->debug() << log_tag() << "<== CHUNK_REQUEST:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " length=" << message_struct.length
		<< " offset=" << message_struct.offset;

	folder_group()->request_block(shared_from_this(), message_struct.ct_hash, message_struct.offset, message_struct.length);
}
void P2PFolder::handle_BlockReply(const blob& message_raw) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto message_struct = parser_.parse_BlockReply(message_raw);
	log_->debug() << log_tag() << "<== CHUNK_REPLY:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " offset=" << message_struct.offset;

	folder_group()->post_chunk(shared_from_this(), message_struct.ct_hash, message_struct.content, message_struct.offset);
}
void P2PFolder::handle_BlockCancel(const blob& message_raw) {
#   warning "Not implemented yet"
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto message_struct = parser_.parse_BlockCancel(message_raw);
	log_->debug() << log_tag() << "<== BLOCK_CANCEL:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " length=" << message_struct.length
		<< " offset=" << message_struct.offset;
}

} /* namespace librevault */
