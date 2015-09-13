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

#include "../../../contrib/crypto/HMAC-SHA3.h"

#include "WireProtocol.pb.h"

namespace librevault {

/*
 * Pascal-like string specifying protocol name. Based on BitTorrent handshake header.
 * Never thought, that "Librevault" and "BitTorrent" have same length.
*/
std::array<char, 19> P2PDirectory::pstr = {'L', 'i', 'b', 'r', 'e', 'v', 'a', 'u', 'l', 't', ' ', 'P', 'r', 'o', 't', 'o', 'c', 'o', 'l'};

P2PDirectory::P2PDirectory(std::unique_ptr<Connection>&& connection, Session& session, Exchanger& exchanger, P2PProvider& provider) :
		AbstractDirectory(session, exchanger), provider_(provider), connection_(std::move(connection)) {
	receive_buffer_ = std::make_shared<blob>();
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
		receive_buffer_->resize(sizeof(protocol_id));
		receive_data();
		connection_->send(generate(PROTOCOL_ID), []{});
	}else if(state == Connection::state::CLOSED){
		disconnect(error);
	}
}

void P2PDirectory::receive_size() {
	awaiting_receive_next_ = SIZE;
	receive_buffer_->resize(sizeof(prefix_t));
	connection_->receive(receive_buffer_, [this]{
		prefix_t size;
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
		protocol_id packet;
		std::copy(receive_buffer_->begin(), receive_buffer_->end(), (byte*)&packet);

		if(packet.pstrlen != pstr.size() || packet.pstr != pstr || packet.version != version) throw protocol_error();

		awaiting_next_ = AWAITING_HANDSHAKE;
		if(connection_->get_role() == Connection::role::CLIENT){
			log_->debug() << "Now we will send handshake message";
			connection_->send(generate(HANDSHAKE), []{});
		}else{
			log_->debug() << "Now we will wait for handshake message";
		}
	}else if(awaiting_next_ == AWAITING_HANDSHAKE){
		if(receive_buffer_->empty() || receive_buffer_->at(0) != HANDSHAKE) throw protocol_error();

		protocol::Handshake handshake;
		if(handshake.ParseFromArray(receive_buffer_->data()+1, receive_buffer_->size()-1)){
			if(connection_->get_role() == Connection::SERVER) {
				blob dir_hash(handshake.dir_hash().begin(), handshake.dir_hash().end());
				attach(dir_hash);
				// TODO: Check remote_token
				connection_->send(generate(HANDSHAKE), []{});
			}

			log_->debug() << "LV Handshake successful";
			awaiting_next_ = AWAITING_ANY;
		}else throw protocol_error();
	}else{

	}
}

blob P2PDirectory::generate(message_type type) {
	// PROTOCOL_ID is one of two non-protobuf messages, so it must be processed differently
	if(type == PROTOCOL_ID){
		protocol_id protocol_id_packet;
		protocol_id_packet.pstrlen = pstr.size();
		protocol_id_packet.pstr = pstr;
		protocol_id_packet.version = version;
		protocol_id_packet.reserved.fill(0);
		return blob((byte*)&protocol_id_packet, (byte*)&protocol_id_packet+sizeof(protocol_id_packet));
	}

	// Other messages are protobuf in this version
	std::shared_ptr<::google::protobuf::Message> proto_message;

	switch(type){
	case HANDSHAKE: {
		std::shared_ptr<protocol::Handshake> handshake = std::make_shared<protocol::Handshake>();
		handshake->set_user_agent(session_.version().user_agent());

		auto dir_hash = directory_ptr_.lock()->hash();
		handshake->set_dir_hash(dir_hash.data(), dir_hash.size());

		auto token = local_token();
		handshake->set_auth_token(token.data(), token.size());

		proto_message = handshake;
	} break;
	default: throw protocol_error();
	}

	blob serialized_message(proto_message->ByteSize());
	proto_message->SerializeToArray(serialized_message.data(), serialized_message.size());

	// Add length and type prefix
	blob packet(sizeof(prefix_t)+sizeof(message_type)+serialized_message.size());
	prefix_t size_be = sizeof(message_type)+serialized_message.size();

	std::copy((uint8_t*)&size_be, ((uint8_t*)&size_be)+sizeof(prefix_t), packet.data());
	packet[sizeof(prefix_t)] = type;
	std::move(serialized_message.begin(), serialized_message.end(), &packet[sizeof(prefix_t)+sizeof(message_type)]);

	log_->debug() << "Generated proto_message type " << type << " content: " << proto_message->DebugString();

	return packet;
}

void P2PDirectory::disconnect(const boost::system::error_code& error) {
	detach();
	log_->debug() << "Connection to " << connection_->remote_string() << " closed: " << error.message();
}

} /* namespace librevault */
