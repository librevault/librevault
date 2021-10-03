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
#include <QString>

#include "util/blob.h"

namespace librevault {

inline QString path_id_readable(const blob& path_id) { return conv_bytearray(path_id).toHex(); }

inline QString path_id_readable(const QByteArray& path_id) { return path_id.toHex(); }

inline QString ct_hash_readable(const blob& ct_hash) { return conv_bytearray(ct_hash).toHex(); }

inline QString ct_hash_readable(const QByteArray& ct_hash) { return ct_hash.toHex(); }

}  // namespace librevault
