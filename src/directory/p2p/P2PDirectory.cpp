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
		directory_sptr->attach_remote(shared_from_this());
	}else throw directory_not_found();
}

void P2PDirectory::detach() {
	auto directory_sptr = directory_ptr_.lock();
	if(directory_sptr) directory_sptr->detach_remote(shared_from_this());
}

void P2PDirectory::handle_establish(Connection::state state, const boost::system::error_code& error) {
	if(state == Connection::state::ESTABLISHED){
		receive_buffer_->resize(sizeof(Protocol_id));
		receive_data();
		connection_->send(gen_protocol_id(), []{});
	}else if(state == Connection::state::CLOSED){
		disconnect(error);
	}
}

void P2PDirectory::receive_size() {
	awaiting_receive_next_ = SIZE;
	receive_buffer_->resize(sizeof(length_prefix_t));
	connection_->receive(receive_buffer_, [this]{
		length_prefix_t size;
		std::copy(receive_buffer_->begin(), receive_buffer_->end(), (uint8_t*)&size);	// receive_buffer_ must contain unsigned big-endian 32bit integer, containing length of data.
		receive_buffer_->resize(size);
		receive_data();
	});
}

void P2PDirectory::receive_data() {
	awaiting_receive_next_ = DATA;
	connection_->receive(receive_buffer_, [this]{handle_message(); receive_size();});
}

void P2PDirectory::handle_message() {
	if(awaiting_next_ == AWAITING_PROTOCOL_ID){
		Protocol_id packet;
		std::copy(receive_buffer_->begin(), receive_buffer_->end(), (byte*)&packet);

		if(packet.pstrlen != pstr.size() || packet.pstr != pstr || packet.version != version) throw protocol_error();

		awaiting_next_ = AWAITING_HANDSHAKE;
		if(connection_->get_role() == Connection::role::CLIENT){
			log_->debug() << "Now we will send handshake message";
			connection_->send(gen_handshake(), []{});
		}else{
			log_->debug() << "Now we will wait for handshake message";
		}
	}else if(awaiting_next_ == AWAITING_HANDSHAKE){
		AbstractParser::Handshake handshake = parser_->parse_handshake(*receive_buffer_);

		if(connection_->get_role() == Connection::SERVER) {
			attach(handshake.dir_hash);
			// TODO: Check remote_token
			connection_->send(gen_handshake(), []{});
		}

		log_->debug() << log_tag() << "LV Handshake successful";
		awaiting_next_ = AWAITING_ANY;
	}
}

blob P2PDirectory::gen_protocol_id() {
	Protocol_id protocol_id_packet;
	protocol_id_packet.pstrlen = pstr.size();
	protocol_id_packet.pstr = pstr;
	protocol_id_packet.version = version;
	protocol_id_packet.reserved.fill(0);
	return blob(protocol_id_packet.raw_bytes.begin(), protocol_id_packet.raw_bytes.end());
}

blob P2PDirectory::gen_handshake() {
	if(!directory_ptr_.lock()) throw protocol_error();

	AbstractParser::Handshake handshake;
	handshake.user_agent = session_.version().user_agent();
	handshake.dir_hash = directory_ptr_.lock()->hash();
	handshake.auth_token = local_token();

	return parser_->gen_handshake(handshake);
}

void P2PDirectory::disconnect(const boost::system::error_code& error) {
	detach();
	log_->debug() << "Connection to " << connection_->remote_string() << " closed: " << error.message();
}

} /* namespace librevault */
