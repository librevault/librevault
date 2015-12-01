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
#include "../Client.h"
#include "AbstractDirectory.h"
#include "Exchanger.h"

#include "../util/bit_reverse.h"

namespace librevault {

AbstractDirectory::AbstractDirectory(Client& client, Exchanger& exchanger) :
		Loggable(client), client_(client), exchanger_(exchanger) {}
AbstractDirectory::~AbstractDirectory() {}

std::string AbstractDirectory::path_id_readable(const blob& path_id) const {
	return crypto::Base32().to_string(path_id);
}

std::string AbstractDirectory::encrypted_data_hash_readable(const blob& block_id) const {
	return crypto::Base32().to_string(block_id);
}

blob AbstractDirectory::convert_bitfield(const bitfield_type& bitfield) {
	blob converted_bitfield;
	converted_bitfield.reserve(bitfield.size()/8);
	boost::to_block_range(bitfield, std::back_inserter(converted_bitfield));
	for(auto& b : converted_bitfield) {  // Reverse each bit, because boost::dynamic_bitfield has 0 as LSB, and size() as MSB.
		b = BitReverseTable256[b];
	}
	return converted_bitfield;
}

AbstractDirectory::bitfield_type AbstractDirectory::convert_bitfield(blob bitfield) {
	bitfield_type converted_bitfield(bitfield.size());
	for(auto& b : bitfield) {
		b = BitReverseTable256[b];
	}
	boost::from_block_range(bitfield.begin(), bitfield.end(), converted_bitfield);
	return converted_bitfield;
}

bool AbstractDirectory::will_accept_meta(const Meta::PathRevision& path_revision) {
	auto it = path_id_info_.find(path_revision.path_id_);
	return (it == path_id_info_.end() || path_revision.revision_ > it->second.first);
}

} /* namespace librevault */
