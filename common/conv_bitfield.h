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
#include <QBitArray>
#include <QByteArray>

namespace librevault {

using bitfield_type = QBitArray;

inline size_t packedSizeUnaligned(size_t bits_unaligned) {
	return (bits_unaligned / 8) + ((bits_unaligned % 8) != 0);
}

inline size_t packedBitfieldSize(const QBitArray& bits) {
	if(bits.size() == 0)	// empty
		return 0;
	if(bits.size() <= 5)	// packed as 1 byte
		return 1;
	return (size_t)1 + packedSizeUnaligned((size_t)bits.size() - 5);	// 5 bits in the first byte, others are just regular
}

inline size_t unpackedBitfieldSize(const QByteArray& bytes) {
	if(bytes.size() == 0)
		return 0;
	uchar bits_significant = (uchar)1 + ((uchar)bytes.at(0) >> 5);	// significant bits in last byte

	if(bytes.size() == 1) {
		if(bits_significant > 5)
			return 0;	// invalid, actually
		return bits_significant;
	}else{
		return (size_t)5 + (bytes.size() * 8 - 2) + bits_significant;
	}
}

inline QByteArray convert_bitfield(const QBitArray& bits) {
	QByteArray bytes(packedBitfieldSize(bits), 0);

	if(bytes.size() == 0)
		return bytes;

	if(bytes.size() == 1) {
		uchar bits_significant = bits.size();
		uchar five_bits = 0;
		for(int i = 0; i < bits.size(); i++) {
			five_bits |= (bits.at(i) << (4-i));
		}
		uchar first_byte = (bits_significant << 5) | five_bits;
	}




	//if(bits.size() == 0)
	//	return QByteArray();

	//int byte_size_unmasked = (bits.size() / 8) + ((bits.size() % 8) ? 1 : 0);
	//QByteArray bytes(byte_size_unmasked + 1, 0);

	//for(int bitn = 0; bitn < bits.size(); bitn++) {
	//	int byten = 1 + (bitn / 8);
	//	char bitmask_current = ((char)bits[bitn] << (7 - (bitn % 8)));
	//	bytes[byten] = bytes[byten] | bitmask_current;
	//}

	//bytes[0] = 0xFF << (8 - (bits.size() % 8));

	return bytes;
}

inline QBitArray convert_bitfield(const QByteArray& bytes) {
	if(bytes.isEmpty())
		return QBitArray();

	QBitArray bits;

	return bits;
}

}
