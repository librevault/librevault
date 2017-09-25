/* Copyright (C) 2015 Alexander Shishenko <alex@shishenko.com>
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
#include "SignedMeta.h"
#include "conv_bitfield.h"
#include <exception_helper.hpp>

namespace librevault {

class V1Parser {
public:
	enum MessageType : uint8_t {
		HANDSHAKE = 0,

		CHOKE = 1,
		UNCHOKE = 2,
		INTERESTED = 3,
		NOTINTERESTED = 4,

		INDEXUPDATE = 5,

		METAREQUEST = 7,
		METARESPONSE = 8,

		BLOCKREQUEST = 10,
		BLOCKRESPONSE = 11,
	};

	struct Handshake {
		QByteArray auth_token;
		QString device_name;
		QString user_agent;
	};
	struct IndexUpdate {
		MetaInfo::PathRevision revision;
		QBitArray bitfield;
	};
	struct MetaRequest {
		MetaInfo::PathRevision revision;
	};
	struct MetaResponse {
		SignedMeta smeta;
		QBitArray bitfield;
	};
	struct BlockRequest {
		QByteArray ct_hash;
		quint32 offset;
		quint32 length;
	};
	struct BlockResponse {
		QByteArray ct_hash;
		quint32 offset;
		QByteArray content;
	};

	/* Errors */
	DECLARE_EXCEPTION(ParseError, "Parse error");

	// gen_* messages return messages in format <type=byte><payload>
	// parse_* messages take argument in format <type=byte><payload>

	MessageType parse_MessageType(const QByteArray& message_raw) {
		if(message_raw.isEmpty())
			throw ParseError();
		return (MessageType)message_raw.at(0);
	}

	QByteArray serializeHandshake(const Handshake& message_struct);
	Handshake parseHandshake(QByteArray message_raw);

	QByteArray genChoke() {return QByteArray(1, CHOKE);}
	QByteArray genUnchoke() {return QByteArray(1, UNCHOKE);}
	QByteArray genInterested() {return QByteArray(1, INTERESTED);}
	QByteArray genNotInterested() {return QByteArray(1, NOTINTERESTED);}

	QByteArray serializeIndexUpdate(const IndexUpdate& message_struct);
	IndexUpdate parseIndexUpdate(QByteArray message_raw);

	QByteArray serializeMetaRequest(const MetaRequest& message_struct);
	MetaRequest parseMetaRequest(QByteArray message_raw);

	QByteArray serializeMetaReply(const MetaResponse& message_struct);
	MetaResponse parseMetaReply(QByteArray message_raw);

	QByteArray serializeBlockRequest(const BlockRequest& message_struct);
	BlockRequest parseBlockRequest(QByteArray message_raw);

	QByteArray serializeBlockResponse(const BlockResponse& message_struct);
	BlockResponse parseBlockResponse(QByteArray message_raw);
};

} /* namespace librevault */
