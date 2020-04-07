/* Written in 2015 by Alexander Shishenko <alex@shishenko.com>
 *
 * LVCrypto - Cryptographic wrapper, used in Librevault.
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication
 * along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#pragma once
#include "Base58.h"

namespace librevault {
namespace crypto {

template <uint8_t N, class InputIterator, class CodePointFromCharLookupTable, class CharFromCodePointLookupTable>
char LuhnMod(InputIterator first, InputIterator last, CodePointFromCharLookupTable cp_from_char_table,
             CharFromCodePointLookupTable char_from_cp_table) {
  int factor = 2;
  int sum = 0;

  for (auto it = --last; it != first; it--) {
    int code_point = cp_from_char_table[uint8_t(*it)];
    int addend = factor * code_point;

    factor = (factor == 2) ? 1 : 2;

    addend = (addend / N) + (addend % N);
    sum += addend;
  }

  int remainder = sum % N;
  int check_code_point = (N - remainder) % N;

  return (char)char_from_cp_table[check_code_point];
}

// clang-format off
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
// clang-format on

template <class InputIterator>
char LuhnMod58(InputIterator first, InputIterator last) {
  return LuhnMod<58>(first, last, LuhnMod58_lookup, Base58_alphabets::bitcoin_alphabet);
}

}  // namespace crypto
}  // namespace librevault
