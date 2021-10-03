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
#include <QFile>

#include "Meta.h"
#include "util/blob.h"
#include "util/conv_bitfield.h"

namespace librevault {

class FolderParams;
class MetaStorage;
class PathNormalizer;

class MemoryStorage;
class EncStorage;
class OpenStorage;
class Archive;
class AssemblerQueue;

class ChunkStorage : public QObject {
  Q_OBJECT
 public:
  struct ChunkNotFound : public std::runtime_error {
    ChunkNotFound() : std::runtime_error("Requested Chunk not found") {}
  };

  ChunkStorage(const FolderParams& params, MetaStorage* meta_storage, PathNormalizer* path_normalizer, QObject* parent);
  ~ChunkStorage() = default;

  [[nodiscard]] bool have_chunk(const QByteArray& ct_hash) const noexcept;
  QByteArray get_chunk(const QByteArray& ct_hash);  // Throws AbstractFolder::ChunkNotFound
  void put_chunk(const QByteArray& ct_hash, QFile* chunk_f);

  bitfield_type make_bitfield(const Meta& meta) const noexcept;  // Bulk version of "have_chunk"

  void cleanup(const Meta& meta);

 signals:
  void chunkAdded(QByteArray ct_hash);

 protected:
  MetaStorage* meta_storage_;

  MemoryStorage* mem_storage;
  EncStorage* enc_storage;
  OpenStorage* open_storage;
  Archive* archive;
  AssemblerQueue* file_assembler;
};

}  // namespace librevault
