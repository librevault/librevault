/* Copyright (C) 2017 Alexander Shishenko <alex@shishenko.com>
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

#include "util/conv_bitfield.h"

namespace librevault {

QBitArray conv_bitarray(const bitfield_type& bitfield) {
  QBitArray bitarray(bitfield.size());
  for (size_t i = 0; i < bitfield.size(); i++) bitarray.setBit(i, bitfield[i]);
  return bitarray;
}

bitfield_type conv_bitarray(const QBitArray& bitarray) {
  bitfield_type bitfield(bitarray.size());
  for (int i = 0; i < bitarray.size(); i++) bitfield[i] = bitarray[i];
  return bitfield;
}

}  // namespace librevault
