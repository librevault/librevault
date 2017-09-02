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
#include "ChunkInfo.h"
#include "ChunkInfo_p.h"

namespace librevault {

ChunkInfo::ChunkInfo() { d = new ChunkInfoPrivate; }
ChunkInfo::ChunkInfo(const ChunkInfo& r) { *this = r; }
ChunkInfo::ChunkInfo(ChunkInfo&& r) noexcept { *this = std::move(r); }
ChunkInfo::~ChunkInfo() {}
ChunkInfo& ChunkInfo::operator=(const ChunkInfo& r) {
  d = r.d;
  return *this;
}
ChunkInfo& ChunkInfo::operator=(ChunkInfo&& r) noexcept {
  d = std::move(r.d);
  return *this;
}

QByteArray ChunkInfo::ctHash() const { return QByteArray::fromStdString(d->proto.ct_hash()); }
void ChunkInfo::ctHash(const QByteArray& ct_hash) { d->proto.set_ct_hash(ct_hash.toStdString()); }

quint64 ChunkInfo::size() const { return d->proto.size(); }
void ChunkInfo::size(quint64 size) { d->proto.set_size(size); }

QByteArray ChunkInfo::iv() const { return QByteArray::fromStdString(d->proto.iv()); }
void ChunkInfo::iv(const QByteArray& iv) { d->proto.set_iv(iv.toStdString()); }

QByteArray ChunkInfo::ptKeyedHash() const { return QByteArray::fromStdString(d->proto.pt_keyed_hash()); }
void ChunkInfo::ptKeyedHash(const QByteArray& pt_keyed_hash) { d->proto.set_pt_keyed_hash(pt_keyed_hash.toStdString()); }

} /* namespace librevault */
