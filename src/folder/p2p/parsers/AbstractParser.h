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
#include <librevault/Meta.h>
#include "src/pch.h"

namespace librevault {

class AbstractParser {
public:
	virtual ~AbstractParser(){}

	enum message_type : byte {
		HANDSHAKE = 0,

		CHOKE = 1,
		UNCHOKE = 2,
		INTERESTED = 3,
		NOT_INTERESTED = 4,

		HAVE_META = 5,
		HAVE_BLOCK = 6,

		META_REQUEST = 7,
		META_REPLY = 8,
		META_CANCEL = 9,

		CHUNK_REQUEST = 10,
		CHUNK_REPLY = 11,
		CHUNK_CANCEL = 12,
	};

	struct Handshake {
		blob dir_hash;
		blob auth_token;
	};
	struct HaveMeta {
		Meta::PathRevision revision;
		bitfield_type bitfield;
	};
	struct HaveBlock {
		blob encrypted_data_hash;
	};
	struct MetaRequest {
		Meta::PathRevision revision;
	};
	struct MetaReply {
		Meta::SignedMeta smeta;
		bitfield_type bitfield;
	};
	struct MetaCancel {
		Meta::PathRevision revision;
	};
	struct ChunkRequest {
		blob encrypted_data_hash;
		uint32_t offset;
		uint32_t length;
	};
	struct ChunkReply {
		blob encrypted_data_hash;
		uint32_t offset;
		blob content;
	};
	struct ChunkCancel {
		blob encrypted_data_hash;
		uint32_t offset;
		uint32_t length;
	};

	/* Errors */
	struct parse_error : std::runtime_error {
		parse_error() : std::runtime_error("Parse error"){}
	};

	// gen_* messages return messages in format <type=byte><payload>
	// parse_* messages take argument in format <type=byte><payload>

	virtual blob gen_Handshake(const Handshake& message_struct) = 0;
	virtual Handshake parse_Handshake(const blob& message_raw) = 0;

	virtual blob gen_HaveMeta(const HaveMeta& message_struct) = 0;
	virtual HaveMeta parse_HaveMeta(const blob& message_raw) = 0;

	virtual blob gen_HaveBlock(const HaveBlock& message_struct) = 0;
	virtual HaveBlock parse_HaveBlock(const blob& message_raw) = 0;

	virtual blob gen_MetaRequest(const MetaRequest& message_struct) = 0;
	virtual MetaRequest parse_MetaRequest(const blob& message_raw) = 0;

	virtual blob gen_MetaReply(const MetaReply& message_struct) = 0;
	virtual MetaReply parse_MetaReply(const blob& message_raw, const Secret& secret_verifier) = 0;

	virtual blob gen_MetaCancel(const MetaCancel& message_struct) = 0;
	virtual MetaCancel parse_MetaCancel(const blob& message_raw) = 0;

	virtual blob gen_ChunkRequest(const ChunkRequest& message_struct) = 0;
	virtual ChunkRequest parse_ChunkRequest(const blob& message_raw) = 0;

	virtual blob gen_ChunkReply(const ChunkReply& message_struct) = 0;
	virtual ChunkReply parse_ChunkReply(const blob& message_raw) = 0;

	virtual blob gen_ChunkCancel(const ChunkCancel& message_struct) = 0;
	virtual ChunkCancel parse_ChunkCancel(const blob& message_raw) = 0;

protected:
	AbstractParser(){}
};

} /* namespace librevault */
