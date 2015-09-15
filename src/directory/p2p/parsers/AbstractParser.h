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
#include "../P2PDirectory.h"

namespace librevault {

class AbstractParser {
public:
	enum message_type : byte {
		HANDSHAKE = 0x00
	};

	struct Handshake {
		std::string user_agent;
		blob dir_hash;
		blob auth_token;
	};

	AbstractParser(){}
	virtual ~AbstractParser(){}

	blob prefix(blob&& unprefixed, message_type type);

	// gen_* messages return messages in format <length=uint32_be><type=byte><payload>
	// parse_* messages take argument in format <type=byte><payload>, without length-prefix

	virtual blob gen_handshake(const Handshake& message_struct) = 0;
	virtual Handshake parse_handshake(const blob& message_raw) = 0;
};

} /* namespace librevault */
