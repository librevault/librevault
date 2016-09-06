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
#include "bit_reverse.h"
#include <boost/dynamic_bitset.hpp>

namespace librevault {

using bitfield_type = boost::dynamic_bitset<uint8_t>;

inline std::vector<uint8_t> convert_bitfield(const bitfield_type& bitfield) {
	std::vector<uint8_t> converted_bitfield;
	converted_bitfield.reserve(bitfield.size()/8);
	boost::to_block_range(bitfield, std::back_inserter(converted_bitfield));
	for(auto byte_it = converted_bitfield.begin(); byte_it < converted_bitfield.end(); ++byte_it) {  // Reverse each bit, because boost::dynamic_bitfield has 0 as LSB, and size() as MSB.
		*byte_it = BitReverseTable256[*byte_it];
	}
	std::reverse(converted_bitfield.begin(), converted_bitfield.end());
	return converted_bitfield;
}

inline bitfield_type convert_bitfield(std::vector<uint8_t> bitfield) {
	bitfield_type converted_bitfield(bitfield.size()*8);
	for(auto& b : bitfield) {
		b = BitReverseTable256[b];
	}
	boost::from_block_range(bitfield.rbegin(), bitfield.rend(), converted_bitfield);
	return converted_bitfield;
}

}