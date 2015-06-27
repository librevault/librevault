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
#include <boost/predef/other.h>

inline uint64_t htonll(uint64_t value){
#if BOOST_ENDIAN_LITTLE_BYTE
	const uint32_t high_part = htonl(static_cast<uint32_t>(value >> 32));
	const uint32_t low_part = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));

	return (static_cast<uint64_t>(low_part) << 32) | high_part;
#else
	return value;
#endif
}

inline uint64_t ntohll(uint64_t value){return htonll(value);}
