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
#include <array>

namespace librevault::btcompat {

using info_hash = std::array<uint8_t, 20>;
using peer_id = std::array<uint8_t, 20>;

inline info_hash getInfoHash(const QByteArray& folderid) {
  info_hash ih;
  std::copy(folderid.begin(), folderid.begin() + std::min(ih.size(), (size_t)folderid.size()), ih.begin());
  return ih;
}

}  // namespace librevault::btcompat
