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
#include "P2PDirectory.h"
#include "P2PProvider.h"
#include "../fs/FSDirectory.h"
#include "../../Session.h"
#include "../Exchanger.h"

#include "parsers/ProtobufParser.h"

namespace librevault {

/*
 * Pascal-like string specifying protocol name. Based on BitTorrent handshake header.
 * Never thought, that "Librevault" and "BitTorrent" have same length.
*/
std::array<char, 19> P2PDirectory::pstr = {'L', 'i', 'b', 'r', 'e', 'v', 'a', 'u', 'l', 't', ' ', 'P', 'r', 'o', 't', 'o', 'c', 'o', 'l'};

P2PDirectory::P2PDirectory(std::unique_ptr<Connection>&& connection, Session& session, Exchanger& exchanger, P2PProvider& provider) :
		AbstractDirectory(session, exchanger), provider_(provider), connection_(std::move(connection)) {
	receive_buffer_ = std::make_shared<blob>();
	parser_ = std::make_unique<ProtobufParser>();
	connection_->establish(std::bind(&P2PDirectory::handle_establish, this, std::placeholders::_1, std::placeholders::_2));
}

P2PDirectory::P2PDirectory(std::unique_ptr<Connection> &&connection, std::shared_ptr<FSDirectory> directory_ptr, Session &session, Exchanger &exchanger, P2PProvider &provider) :
		P2PDirectory(std::move(connection), session, exchanger, provider) {
	directory_ptr_ = directory_ptr;
}

P2PDirectory::~P2PDirectory() {}

blob P2PDirectory::local_token() {
	return crypto::HMAC_SHA3_224(directory_ptr_.lock()->key().get_Public_Key()).to(provider_.node_key().public_key());
}

blob P2PDirectory::remote_token() {
	return crypto::HMAC_SHA3_224(directory_ptr_.lock()->key().get_Public_Key()).to(connection_->remote_pubkey());
}

void P2PDirectory::attach(const blob& dir_hash) {
	auto directory_sptr = exchanger_.get_directory(dir_hash);
	if(directory_sptr){
		directory_ptr_ = directory_sptr;
		directory_sptr->attach_p2p_dir(shared_from_this());
		provider_.remove_from_unattached(shared_from_this());
	}else throw directory_not_found();
}

void P2PDirectory::detach() {
	auto directory_sptr = directory_ptr_.lock();
	if(directory_sptr) directory_sptr->detach_p2p_dir(shared_from_this());
}

void P2PDirectory::handle_establish(Connection::state state, const boost::system::error_code& error) {
	if(state == Connection::state::ESTABLISHED){
		receive_buffer_->resize(sizeof(protocol_tag));
		receive_data();
		send_protocol_tag();
	}else if(state == Connection::state::CLOSED){
		disconnect(error);
	}
}

void P2PDirectory::receive_size() {
	awaiting_receive_next_ = SIZE;
	receive_buffer_->resize(sizeof(length_prefix_t));

	log_->debug() << log_tag() << "Ready to receive length-prefix!";

	connection_->receive(receive_buffer_, [this]{
		length_prefix_t size_big;
		std::copy(receive_buffer_->begin(), receive_buffer_->end(), (uint8_t*)&size_big);	// receive_buffer_ must contain unsigned big-endian 32bit integer, containing length of data.
		uint32_t size = size_big;

		log_->debug() << log_tag() << "Received length-prefix: " << size;

		receive_buffer_->resize(size);
		receive_data();
	});
}

void P2PDirectory::receive_data() {
	awaiting_receive_next_ = DATA;
	connection_->receive(receive_buffer_, [this]{
		handle_message();
		receive_size();
	});
}

void P2PDirectory::handle_message() {
	try {
		if(awaiting_next_ != AWAITING_ANY){
			// Handshake sequence

			switch(awaiting_next_) {
				case AWAITING_PROTOCOL_TAG: {
					protocol_tag packet;
					std::copy(receive_buffer_->begin(), receive_buffer_->end(), (byte*)&packet);

					if(packet.pstrlen != pstr.size() || packet.pstr != pstr || packet.version != version)
						throw protocol_error();

					log_->debug() << log_tag() << "Received correct protocol_tag";

					awaiting_next_ = AWAITING_HANDSHAKE;
					if(connection_->get_role() == Connection::role::CLIENT){
						log_->debug() << "Now we will send handshake message";
						send_handshake();
					}else {
						log_->debug() << "Now we will wait for handshake message";
					}
				}
					break;
				case AWAITING_HANDSHAKE: {
					AbstractParser::Handshake handshake = parser_->parse_handshake(*receive_buffer_);    // Must check message_type and so on.

					attach(handshake.dir_hash);
					if(handshake.auth_token != remote_token()) throw auth_error();

					if(connection_->get_role() == Connection::SERVER) send_handshake();

					log_->debug() << log_tag() << "LV Handshake successful";
					awaiting_next_ = AWAITING_ANY;

					send_meta_list();
				}
					break;
				default: throw protocol_error();
			}
		}else {
			// Regular message flow

			AbstractParser::message_type message_type;
			if(receive_buffer_->empty())
				message_type = AbstractParser::KEEP_ALIVE;
			else
				message_type = (AbstractParser::message_type)receive_buffer_->at(0);

			switch(message_type) {
				case AbstractParser::META_LIST: {
					log_->debug() << log_tag() << "Received meta_list";
					AbstractParser::MetaList
						meta_list = parser_->parse_meta_list(*receive_buffer_);    // Must check message_type and so on.

					for(auto revision : meta_list.revision) {
						revisions_.insert({revision.path_id_, revision.revision_});
						directory_ptr_.lock()->post_revision(shared_from_this(), revision);
					}
				}
					break;
				case AbstractParser::META_REQUEST: {
					AbstractParser::MetaRequest meta_request =
						parser_->parse_meta_request(*receive_buffer_);    // Must check message_type and so on.
					log_->debug() << log_tag() << "Received meta_request " << path_id_readable(meta_request.path_id);
					directory_ptr_.lock()->request_meta(shared_from_this(), meta_request.path_id);
				}
					break;
				case AbstractParser::META: {
					AbstractParser::Meta
						meta = parser_->parse_meta(*receive_buffer_);    // Must check message_type and so on.
					log_->debug() << log_tag() << "Received meta";
					directory_ptr_.lock()->post_meta(shared_from_this(), meta.smeta);
				}
					break;
				case AbstractParser::BLOCK_REQUEST: {
					AbstractParser::BlockRequest block_request =
						parser_->parse_block_request(*receive_buffer_);    // Must check message_type and so on.
					log_->debug() << log_tag() << "Received block_request "
						<< encrypted_data_hash_readable(block_request
															.block_id);
					directory_ptr_.lock()->request_block(shared_from_this(), block_request.block_id);
				}
					break;
				case AbstractParser::BLOCK: {
					AbstractParser::Block
						block = parser_->parse_block(*receive_buffer_);    // Must check message_type and so on.
					log_->debug() << log_tag() << "Received block " << encrypted_data_hash_readable(block.block_id);
					directory_ptr_.lock()->post_block(shared_from_this(), block.block_id, block.block_content);
				}
					break;
				default: throw protocol_error();
			}
		}
	}catch(std::runtime_error& e){
		log_->warn() << log_tag() << "Closing connection because of exception: " << e.what();
		disconnect();
	}
}

std::vector<Meta::PathRevision> P2PDirectory::get_meta_list() {
	std::vector<Meta::PathRevision> result;
	for(auto& map_revision : revisions_){
		Meta::PathRevision path_revision;
		path_revision.path_id_ = map_revision.first;
		path_revision.revision_ = map_revision.second;
		result.push_back(path_revision);
	}
	return result;
}

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

void P2PDirectory::post_meta(std::shared_ptr<AbstractDirectory> origin, const SignedMeta& smeta) {
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
}

void P2PDirectory::send_protocol_tag() {
	protocol_tag message;
	message.pstrlen = pstr.size();
	message.pstr = pstr;
	message.version = version;
	message.reserved.fill(0);

	log_->debug() << log_tag() << "Sending protocol_tag";
	connection_->send(blob(message.raw_bytes.begin(), message.raw_bytes.end()), []{});
}

void P2PDirectory::send_handshake() {
	if(!directory_ptr_.lock()) throw protocol_error();

	AbstractParser::Handshake handshake;
	handshake.user_agent = session_.version().user_agent();
	handshake.dir_hash = directory_ptr_.lock()->hash();
	handshake.auth_token = local_token();

	log_->debug() << log_tag() << "Sending handshake";
	connection_->send(parser_->gen_handshake(handshake), []{});
}

void P2PDirectory::send_meta_list() {
	AbstractParser::MetaList meta_list;
	meta_list.revision = directory_ptr_.lock()->get_meta_list();

	log_->debug() << log_tag() << "Sending meta_list";
	connection_->send(parser_->gen_meta_list(meta_list), []{});
}

void P2PDirectory::disconnect(const boost::system::error_code& error) {
	provider_.remove_from_unattached(shared_from_this());
	detach();
	log_->debug() << "Connection to " << connection_->remote_string() << " closed: " << error.message();
}

} /* namespace librevault */
