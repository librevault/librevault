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
#pragma once
#include "AbstractParser.h"

namespace librevault {

class ProtobufParser : public AbstractParser {
public:
	ProtobufParser(){}
	virtual ~ProtobufParser(){}

	virtual blob gen_Handshake(const Handshake& message_struct);
	virtual Handshake parse_Handshake(const blob& message_raw);

	virtual blob gen_HaveMeta(const HaveMeta& message_struct);
	virtual HaveMeta parse_HaveMeta(const blob& message_raw);

	virtual blob gen_HaveBlock(const HaveBlock& message_struct);
	virtual HaveBlock parse_HaveBlock(const blob& message_raw);

	virtual blob gen_MetaRequest(const MetaRequest& message_struct);
	virtual MetaRequest parse_MetaRequest(const blob& message_raw);

	virtual blob gen_MetaReply(const MetaReply& message_struct);
	virtual MetaReply parse_MetaReply(const blob& message_raw, const Secret& secret_verifier);

	virtual blob gen_MetaCancel(const MetaCancel& message_struct);
	virtual MetaCancel parse_MetaCancel(const blob& message_raw);

	virtual blob gen_ChunkRequest(const ChunkRequest& message_struct);
	virtual ChunkRequest parse_ChunkRequest(const blob& message_raw);

	virtual blob gen_ChunkReply(const ChunkReply& message_struct);
	virtual ChunkReply parse_ChunkReply(const blob& message_raw);

	virtual blob gen_ChunkCancel(const ChunkCancel& message_struct);
	virtual ChunkCancel parse_ChunkCancel(const blob& message_raw);

private:
	template <class ProtoMessageClass>
	blob prepare_proto_message(const ProtoMessageClass& message_protobuf, message_type type) {
		blob message_raw(message_protobuf.ByteSize()+1);
		message_raw[0] = type;
		message_protobuf.SerializeToArray(message_raw.data()+1, message_protobuf.ByteSize());

		return message_raw;
	}
};

} /* namespace librevault */
