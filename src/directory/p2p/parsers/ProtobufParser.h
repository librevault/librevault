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
#pragma once
#include "AbstractParser.h"

namespace librevault {

class ProtobufParser : public AbstractParser {
public:
	ProtobufParser(){}
	virtual ~ProtobufParser(){}

	blob gen_handshake(const Handshake& message_struct);
	Handshake parse_handshake(const blob& message_raw);

	blob gen_meta_list(const MetaList& message_struct);
	MetaList parse_meta_list(const blob& message_raw);

	blob gen_meta_request(const MetaRequest& message_struct);
	MetaRequest parse_meta_request(const blob& message_raw);

	blob gen_meta(const Meta& message_struct);
	Meta parse_meta(const blob& message_raw);

	blob gen_block_request(const BlockRequest& message_struct);
	BlockRequest parse_block_request(const blob& message_raw);

	blob gen_block(const Block& message_struct);
	Block parse_block(const blob& message_raw);

private:
	template <class ProtoMessageClass>
	blob prepare_proto_message(const ProtoMessageClass& message_protobuf, message_type type) {
		blob message_raw(message_protobuf.ByteSize());
		message_protobuf.SerializeToArray(message_raw.data(), message_raw.size());

		return prefix(std::move(message_raw), type);
	}
};

} /* namespace librevault */
