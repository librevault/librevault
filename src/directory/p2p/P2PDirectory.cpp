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

/*
 * Pascal-like string specifying protocol name. Based on BitTorrent handshake header.
 * Never thought, that "Librevault" and "BitTorrent" have same length.
*/
std::array<char, 19> P2PDirectory::pstr = {'L', 'i', 'b', 'r', 'e', 'v', 'a', 'u', 'l', 't', ' ', 'P', 'r', 'o', 't', 'o', 'c', 'o', 'l'};

P2PDirectory::P2PDirectory(std::unique_ptr<Connection>&& connection, Client& client, Exchanger& exchanger, P2PProvider& provider) :
		RemoteDirectory(client, exchanger),
		provider_(provider),
		connection_(std::move(connection)),
		timeout(client.ios()) {
	log_->trace() << log_tag() << "Created P2PDirectory";
	receive_buffer_ = std::make_shared<blob>();
	parser_ = std::make_unique<ProtobufParser>();
	connection_->establish(std::bind(&P2PDirectory::handle_establish, this, std::placeholders::_1, std::placeholders::_2));
}

P2PDirectory::P2PDirectory(std::unique_ptr<Connection> &&connection, std::shared_ptr<ExchangeGroup> exchange_group, Client& client, Exchanger &exchanger, P2PProvider &provider) :
		P2PDirectory(std::move(connection), client, exchanger, provider) {
	exchange_group_ = exchange_group;
}

P2PDirectory::~P2PDirectory() {}

blob P2PDirectory::local_token() {
	return provider_.node_key().public_key() | crypto::HMAC_SHA3_224(exchange_group_.lock()->key().get_Public_Key());
}

blob P2PDirectory::remote_token() {
	return connection_->remote_pubkey() | crypto::HMAC_SHA3_224(exchange_group_.lock()->key().get_Public_Key());
}

/* RPC Actions */
void P2PDirectory::choke() {
	blob message = {0,0,0,1,AbstractParser::CHOKE};
	connection_->send(message, []{});
	log_->debug() << log_tag() << "==> CHOKE";
}
void P2PDirectory::unchoke() {
	blob message = {0,0,0,1,AbstractParser::UNCHOKE};
	connection_->send(message, []{});
	log_->debug() << log_tag() << "==> UNCHOKE";
}
void P2PDirectory::interest() {
	blob message = {0,0,0,1,AbstractParser::INTERESTED};
	connection_->send(message, []{});
	log_->debug() << log_tag() << "==> INTERESTED";
}
void P2PDirectory::uninterest() {
	blob message = {0,0,0,1,AbstractParser::NOT_INTERESTED};
	connection_->send(message, []{});
	log_->debug() << log_tag() << "==> NOT_INTERESTED";
}

void P2PDirectory::post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	AbstractParser::HaveMeta message;
	message.revision = revision;
	message.bitfield = convert_bitfield(bitfield);
	connection_->send(parser_->gen_HaveMeta(message), []{});
}
void P2PDirectory::post_have_block(const blob& encrypted_data_hash) {
	AbstractParser::HaveBlock message;
	message.encrypted_data_hash = encrypted_data_hash;
	connection_->send(parser_->gen_HaveBlock(message), []{});
}

void P2PDirectory::request_meta(const Meta::PathRevision& revision) {
	AbstractParser::MetaRequest message;
	message.revision = revision;
	connection_->send(parser_->gen_MetaRequest(message), []{});
}
void P2PDirectory::post_meta(const Meta::SignedMeta& smeta) {
	AbstractParser::MetaReply message;
	message.smeta = smeta;
	connection_->send(parser_->gen_MetaReply(message), []{});
}
void P2PDirectory::cancel_meta(const Meta::PathRevision& revision) {
	AbstractParser::MetaCancel message;
	message.revision = revision;
	connection_->send(parser_->gen_MetaCancel(message), []{});
}

void P2PDirectory::request_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t length) {
	AbstractParser::ChunkRequest message;
	message.encrypted_data_hash = encrypted_data_hash;
	message.offset = offset;
	message.length = length;
	connection_->send(parser_->gen_ChunkRequest(message), []{});
}
void P2PDirectory::post_chunk(const blob& encrypted_data_hash, uint32_t offset, const blob& chunk) {
	AbstractParser::ChunkReply message;
	message.encrypted_data_hash = encrypted_data_hash;
	message.offset = offset;
	message.content = chunk;
	connection_->send(parser_->gen_ChunkReply(message), []{});
}
void P2PDirectory::cancel_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t length) {
	AbstractParser::ChunkCancel message;
	message.encrypted_data_hash = encrypted_data_hash;
	message.offset = offset;
	message.length = length;
	connection_->send(parser_->gen_ChunkCancel(message), []{});
}

void P2PDirectory::handle_establish(Connection::state state, const boost::system::error_code& error) {
	if(state == Connection::state::ESTABLISHED){
		if(provider_.node_key().public_key() == remote_pubkey()){
			provider_.mark_loopback(remote_endpoint());
			return disconnect();
		}

		receive_buffer_->resize(sizeof(protocol_tag));
		receive_data();
		send_protocol_tag();
	}else if(state == Connection::state::CLOSED){
		disconnect(error);
	}
}

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
}

void P2PDirectory::receive_size() {
	receive_buffer_->resize(sizeof(length_prefix_t));

	log_->debug() << log_tag() << "Ready to receive length-prefix!";

	connection_->receive(receive_buffer_, [this]{
		bump_timeout();
		length_prefix_t size_big;
		std::copy(receive_buffer_->begin(), receive_buffer_->end(), (uint8_t*)&size_big);	// receive_buffer_ must contain unsigned big-endian 32bit integer, containing length of data.
		uint32_t size = size_big;

		log_->debug() << log_tag() << "Received length-prefix: " << size;

		receive_buffer_->resize(size);
		receive_data();
	});
}

void P2PDirectory::receive_data() {
	connection_->receive(receive_buffer_, [this]{
		bump_timeout();
		try {
			handle_message();
			receive_size();
		}catch(std::runtime_error& e){
			log_->warn() << log_tag() << "Closing connection because of exception: " << e.what();
			disconnect();
		}
	});
}

void P2PDirectory::bump_timeout() {}

void P2PDirectory::handle_message() {
	if(!is_handshaken()) {    // Handshake sequence
		switch(awaiting_next_) {
			case AWAITING_PROTOCOL_TAG: handle_protocol_tag(); break;
			case AWAITING_HANDSHAKE: handle_Handshake(); break;
			default: throw protocol_error();
		}
	}else{  // Regular message flow
		AbstractParser::message_type message_type;
		if(receive_buffer_->empty())
			message_type = AbstractParser::KEEP_ALIVE;
		else
			message_type = (AbstractParser::message_type)receive_buffer_->at(0);

		switch(message_type) {
			case AbstractParser::CHOKE: handle_Choke(); break;
			case AbstractParser::UNCHOKE: handle_Unchoke(); break;
			case AbstractParser::INTERESTED: handle_Interested(); break;
			case AbstractParser::NOT_INTERESTED: handle_NotInterested(); break;
			case AbstractParser::HAVE_META: handle_HaveMeta(); break;
			case AbstractParser::HAVE_BLOCK: handle_HaveBlock(); break;
			case AbstractParser::META_REQUEST: handle_MetaRequest(); break;
			case AbstractParser::META_REPLY: handle_MetaReply(); break;
			case AbstractParser::META_CANCEL: handle_MetaCancel(); break;
			case AbstractParser::CHUNK_REQUEST: handle_ChunkRequest(); break;
			case AbstractParser::CHUNK_REPLY: handle_ChunkReply(); break;
			case AbstractParser::CHUNK_CANCEL: handle_ChunkCancel(); break;
			case AbstractParser::KEEP_ALIVE: break;
			default: throw protocol_error();
		}
	}
}

void P2PDirectory::handle_protocol_tag() {
	protocol_tag packet;
	std::copy(receive_buffer_->begin(), receive_buffer_->end(), (byte*)&packet);

	if(packet.pstrlen != pstr.size() || packet.pstr != pstr || packet.version != version)
		throw protocol_error();

	log_->debug() << log_tag() << "Received correct protocol_tag";

	awaiting_next_ = AWAITING_HANDSHAKE;
	if(connection_->get_role() == Connection::role::CLIENT){
		log_->debug() << "Now we will send handshake message";
		send_Handshake();
	}else {
		log_->debug() << "Now we will wait for handshake message";
	}
}

void P2PDirectory::handle_Handshake() {
	AbstractParser::Handshake handshake = parser_->parse_Handshake(*receive_buffer_);

	// Attaching to ExchangeGroup
	auto group_ptr = exchanger_.get_group(handshake.dir_hash);
	if(group_ptr)
		group_ptr->attach(shared_from_this());
	else throw ExchangeGroup::attach_error();

	// Checking authentication using token
	if(handshake.auth_token != remote_token()) throw auth_error();

	if(connection_->get_role() == Connection::SERVER) send_Handshake();

	log_->debug() << log_tag() << "LV Handshake successful";
	awaiting_next_ = AWAITING_ANY;

	send_meta_list();
}

void P2PDirectory::handle_Choke() {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_Unchoke() {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_Interested() {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_NotInterested() {
#   warning "Not implemented yet"
}

void P2PDirectory::handle_HaveMeta() {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_HaveBlock() {
#   warning "Not implemented yet"
}

void P2PDirectory::handle_MetaRequest() {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_MetaReply() {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_MetaCancel() {
#   warning "Not implemented yet"
}

void P2PDirectory::handle_ChunkRequest() {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_ChunkReply() {
#   warning "Not implemented yet"
}
void P2PDirectory::handle_ChunkCancel() {
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

void P2PDirectory::send_protocol_tag() {
	protocol_tag message;
	message.pstrlen = pstr.size();
	message.pstr = pstr;
	message.version = version;
	message.reserved.fill(0);

	log_->debug() << log_tag() << "Sending protocol_tag";
	connection_->send(blob(message.raw_bytes.begin(), message.raw_bytes.end()), []{});
}

void P2PDirectory::send_Handshake() {
	if(!exchange_group_.lock()) throw protocol_error();

	AbstractParser::Handshake handshake;
	handshake.dir_hash = exchange_group_.lock()->hash();
	handshake.auth_token = local_token();
	handshake.user_agent = client_.version().user_agent();

	log_->debug() << log_tag() << "Sending handshake";
	connection_->send(parser_->gen_Handshake(handshake), []{});
}

void P2PDirectory::send_meta_list() {
#   warning "Not implemented yet"
}

} /* namespace librevault */
