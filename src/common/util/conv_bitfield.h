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
#include <cstdint>
#include <vector>

namespace librevault {

using bitfield_type = std::vector<bool>;

inline std::vector<uint8_t> convert_bitfield(const bitfield_type& bits) {
  size_t byte_size = (bits.size() / 8) + ((bits.size() % 8) ? 1 : 0);
  std::vector<uint8_t> bytes(byte_size);

  for (size_t bitn = 0; bitn < bits.size(); bitn++) bytes[bitn / 8] |= ((bits[bitn] ? 1u : 0u) << (7 - (bitn % 8)));

  return bytes;
}

inline bitfield_type convert_bitfield(const std::vector<uint8_t>& bytes) {
  bitfield_type bits(bytes.size() * 8);
  for (size_t byten = 0; byten < bytes.size(); byten++)
    for (size_t bitn = 0; bitn < 8; bitn++) bits[byten * 8 + bitn] = (bytes[byten] & (1u << (7 - bitn)));
  return bits;
}

}  // namespace librevault
