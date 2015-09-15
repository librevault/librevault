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
#include "AbstractParser.h"
#include "../P2PDirectory.h"

namespace librevault {

blob AbstractParser::prefix(blob&& unprefixed, message_type type) {
	blob message_raw(sizeof(P2PDirectory::length_prefix_t)+sizeof(message_type)+unprefixed.size());

	auto length_pos = message_raw.begin();
	auto type_pos = length_pos + sizeof(P2PDirectory::length_prefix_t);
	auto payload_pos = type_pos + sizeof(message_type);

	P2PDirectory::length_prefix_t length_prefix = sizeof(message_type)+unprefixed.size();

	std::copy((byte*)&length_prefix, ((byte*)&length_prefix)+sizeof(P2PDirectory::length_prefix_t), length_pos);
	*type_pos = type;
	std::move(unprefixed.begin(), unprefixed.end(), payload_pos);

	return message_raw;
}

} /* namespace librevault */
