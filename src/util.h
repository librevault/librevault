/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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
#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_

#include <iostream>
#include <cstdint>
#include <string>
#include <array>
#include <vector>

namespace librevault {

constexpr size_t SHASH_LENGTH = 28;
constexpr size_t AES_BLOCKSIZE = 16;
constexpr size_t AES_KEYSIZE = 32;

using key_t = std::array<uint8_t, AES_KEYSIZE>;
using iv_t = std::array<uint8_t, AES_BLOCKSIZE>;
using shash_t = std::array<uint8_t, SHASH_LENGTH>;

uint64_t filesize(std::istream& ifile);
uint64_t filesize(std::ostream& ofile);

std::vector<uint8_t> encrypt(const uint8_t* data, size_t size, key_t key, iv_t iv, bool nopad = false);
std::vector<uint8_t> decrypt(const uint8_t* data, size_t size, key_t key, iv_t iv, bool nopad = false);

std::string to_hex(const uint8_t* data, size_t size);
std::string to_hex(const std::string& s);
std::string to_hex(const uint32_t& s);

std::string to_base32(const uint8_t* data, size_t size);
std::vector<uint8_t> from_base32(const std::string& data);

shash_t compute_shash(const uint8_t* data, size_t size);
shash_t compute_hmac(const uint8_t* data, size_t size, key_t key);

} /* namespace librevault */

#endif /* SRC_UTIL_H_ */
