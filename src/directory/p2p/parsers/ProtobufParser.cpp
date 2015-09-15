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

	blob message_raw(message_protobuf.ByteSize());
	message_protobuf.SerializeToArray(message_raw.data(), message_raw.size());

	return prefix(std::move(message_raw), HANDSHAKE);
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

} /* namespace librevault */
