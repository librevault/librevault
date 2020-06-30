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
#include <cstdint>
#include <vector>

namespace librevault {

inline QByteArray convert_bitfield(const QBitArray& bits) {
  int byte_size = (bits.size() / 8) + ((bits.size() % 8) ? 1 : 0);
  QByteArray bytes(byte_size, '\0');

  for (int bitn = 0; bitn < bits.size(); bitn++) {
    uchar b = bytes[bitn / 8];
    b |= uchar((bits[bitn] ? 1u : 0u) << (7 - (bitn % 8)));
    bytes[bitn / 8] = b;
  }

  return bytes;
}

inline QBitArray convert_bitfield(const QByteArray& bytes) {
  QBitArray bits(bytes.size() * 8);
  for (int byten = 0; byten < bytes.size(); byten++)
    for (int bitn = 0; bitn < 8; bitn++) bits[byten * 8 + bitn] = (uchar(bytes[byten]) & (1u << (7 - bitn)));
  return bits;
}

}  // namespace librevault
