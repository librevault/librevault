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
#include "MemoryStorage.h"

#include "ChunkStorage.h"

namespace librevault {

MemoryStorage::MemoryStorage(QObject* parent)
    : QObject(parent), cache_(50 * 1024 * 1024) {}  // 50 MB cache is enough for most purposes

bool MemoryStorage::have_chunk(const QByteArray& ct_hash) const noexcept { return cache_.contains(ct_hash); }

QByteArray MemoryStorage::get_chunk(const QByteArray& ct_hash) const {
  QMutexLocker lk(&cache_lock_);

  QByteArray* cached_chunk = cache_[ct_hash];
  if (cached_chunk)
    return *cached_chunk;
  else
    throw ChunkStorage::ChunkNotFound();
}

void MemoryStorage::put_chunk(const QByteArray& ct_hash, QByteArray data) {
  QMutexLocker lk(&cache_lock_);

  QByteArray* cached_chunk = new QByteArray(data);
  cache_.insert(ct_hash, cached_chunk, cached_chunk->size());
}

}  // namespace librevault
