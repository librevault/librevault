/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
#include <cryptopp/integer.h>

#include <QByteArray>
#include <cstdint>
#include <vector>

namespace librevault {

using blob = std::vector<uint8_t>;

inline QByteArray conv_bytearray(const blob& bl) {
  Q_ASSERT(bl.size() <= INTMAX_MAX);
  return QByteArray((const char*)bl.data(), (int)bl.size());
}

inline blob conv_bytearray(const QByteArray& ba) { return blob(ba.begin(), ba.end()); }

inline CryptoPP::Integer conv_bytearray_to_integer(const QByteArray& ba) {
  return CryptoPP::Integer((uchar*)ba.data(), ba.size());
}

}  // namespace librevault
