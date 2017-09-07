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
#include "conv_bitfield.h"

namespace librevault {

namespace {

size_t packedSizeUnaligned(size_t bits_unaligned) {
	return (bits_unaligned / 8) + ((bits_unaligned % 8) != 0);
}

size_t packedBitfieldSize(const QBitArray& bits) {
	if(bits.size() == 0)  // empty
		return 0;
	if(bits.size() <= 5)  // packed as 1 byte
		return 1;
	return (size_t)1 + packedSizeUnaligned((size_t)bits.size() - 5);  // 5 bits in the first byte, others are just regular
}

size_t unpackedBitfieldSize(const QByteArray& bytes) {
	if(bytes.size() == 0)
		return 0;
	uchar bits_significant = (uchar)1 + ((uchar)bytes.at(0) >> 5);  // significant bits in last byte

	if(bytes.size() == 1) {
		if(bits_significant > 5)
			return 0;  // invalid, actually
		return bits_significant;
	}else {
		return (size_t)5 + (bytes.size() * 8 - 2) + bits_significant;
	}
}

char packHeader(const QBitArray& bits) {
	Q_ASSERT(bits.size() != 0);
	return bits.size() <= 5 ? char(bits.size()) << 5 : char((bits.size() - 5) % 8) << 5;
}

}

QByteArray convert_bitfield(const QBitArray& bits) {
	if(bits.size() == 0)
		return QByteArray();

	QByteArray result(packedBitfieldSize(bits), 0);

	int shift_now = 4;	// 7..0
	int byte_now = 0;		// 0..x
	for(int i = 0; i < bits.size(); i++) {
		result[byte_now] = result[byte_now] | bool(bits[i]) << shift_now;
		if(shift_now-- == 0) {
			shift_now = 7;
			byte_now++;
		}
	}

	result[0] = result[0] | packHeader(bits);
	return result;
}

QBitArray convert_bitfield(const QByteArray& bytes) {
	if(bytes.size() == 0)
		return QBitArray();

	QBitArray result(unpackedBitfieldSize(bytes));
	int bit_now = 3;		// 0..7
	int byte_now = 0;		// 0..x
	for(auto i = 0; i < result.size(); i++) {
		result.setBit (i, bytes[byte_now] & (1 << (7 - bit_now)));
		if(bit_now++ == 7) {
			bit_now = 0;
			byte_now++;
		}
	}
	return result;
}

}
