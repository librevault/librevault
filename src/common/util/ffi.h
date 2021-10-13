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
#include <librevault-rs/src/lib.rs.h>

namespace librevault {

inline FfiConstBuffer from_cpp(const QByteArray& ba) {
  return {reinterpret_cast<const uint8_t*>(ba.data()), static_cast<uintptr_t>(ba.size())};
}

inline QByteArray from_rust(FfiConstBuffer buf) {
  if(buf.str_p != nullptr) {
    auto buf_copy = QByteArray{reinterpret_cast<const char*>(buf.str_p), static_cast<int>(buf.str_len)};
    drop_ffi(buf);
    return buf_copy;
  }
  return {};
}

inline rust::Slice<const uint8_t> to_slice(const QByteArray& data) {
  return { reinterpret_cast<const uint8_t*>(data.data()), static_cast<size_t>(data.size())};
}

inline QByteArray from_vec(const rust::Vec<uint8_t>& data) {
  return {reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()) };
}

}  // namespace librevault
