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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#pragma once
#include "Base58.h"

namespace librevault {

template<uint8_t N, class InputIterator, class CodePointFromCharLookupTable, class CharFromCodePointLookupTable>
char LuhnMod(InputIterator first,
		InputIterator last,
		CodePointFromCharLookupTable cp_from_char_table,
		CharFromCodePointLookupTable char_from_cp_table) {
	int factor = 2;
	int sum = 0;

	for(auto it = --last; it != first; it--){
		int code_point = cp_from_char_table[uint8_t(*it)];
		int addend = factor * code_point;

		factor = (factor == 2) ? 1 : 2;

		addend = (addend / N) + (addend % N);
		sum += addend;
	}

	int remainder = sum % N;
	int check_code_point = (N-remainder) % N;

	return (char)char_from_cp_table[check_code_point];
}

static uint8_t LuhnMod58_lookup[128] = {
//		  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  A,  B,  C,  D,  E,  F
/*0*/	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
/*1*/	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
/*2*/	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
/*3*/	255,  0,  1,  2,  3,  4,  5,  6,  7,  8,255,255,255,255,255,255,
/*4*/	255,  9, 10, 11, 12, 13, 14, 15, 16,255, 17, 18, 19, 20, 21,255,
/*5*/	 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,255,255,255,255,255,
/*6*/	255, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,255, 44, 45, 46,
/*7*/	 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,255,255,255,255,255
};	// Lookup table for Bitcoin Base58 alphabet. 255 = Undefined

template<class InputIterator>
char LuhnMod58(InputIterator first, InputIterator last) {
	return LuhnMod<58>(first, last, LuhnMod58_lookup, base58_alphabet);
}

} /* namespace librevault */
