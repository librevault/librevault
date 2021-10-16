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

#include <QByteArray>
#include <librevaultrs.hpp>
#include <librevault_util/src/lib.rs.h>

namespace librevault {

inline rust::Slice<const uint8_t> to_slice(const QByteArray& data) {
  return { reinterpret_cast<const uint8_t*>(data.data()), static_cast<size_t>(data.size())};
}

inline QByteArray from_vec(const rust::Vec<uint8_t>& data) {
  return {reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()) };
}

}  // namespace librevault
