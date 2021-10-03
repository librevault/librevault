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
#include <QCache>
#include <QFile>

#include "util/AvailabilityMap.h"
#include "util/blob.h"

namespace librevault {

/* ChunkFileBuilderFdPool is a singleton class, used to open/close files automatically to reduce simultaneously open
 * file descriptors */
class ChunkFileBuilderFdPool {
 public:
  static ChunkFileBuilderFdPool* get_instance() {
    static ChunkFileBuilderFdPool* instance;
    if (!instance) instance = new ChunkFileBuilderFdPool();
    return instance;
  }

  QFile* getFile(QString path, bool release = false);

 private:
  QCache<QString, QFile> opened_files_;
};

/* ChunkFileBuilder constructs a chunk in a file. If complete(), then an encrypted chunk is located in  */
class ChunkFileBuilder {
 public:
  ChunkFileBuilder(QString system_path, QByteArray ct_hash, quint32 size);
  ~ChunkFileBuilder();

  QFile* release_chunk();
  void put_block(quint32 offset, const QByteArray& content);

  uint64_t size() const { return file_map_.size_original(); }
  bool complete() const { return file_map_.full(); }

  const AvailabilityMap<quint32>& file_map() const { return file_map_; }

 private:
  AvailabilityMap<quint32> file_map_;
  QString chunk_location_;
};

}  // namespace librevault
