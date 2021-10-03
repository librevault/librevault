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
#include <QObject>
#include <QRunnable>

#include "SignedMeta.h"
#include "util/blob.h"

namespace librevault {

class PathNormalizer;

class Archive;
class MetaStorage;
class FolderParams;
class ChunkStorage;
class Secret;

class AssemblerWorker : public QObject, public QRunnable {
 public:
  struct abort_assembly : std::runtime_error {
    explicit abort_assembly() : std::runtime_error("Assembly aborted") {}
  };

  AssemblerWorker(SignedMeta smeta, const FolderParams& params, MetaStorage* meta_storage, ChunkStorage* chunk_storage,
                  PathNormalizer* path_normalizer, Archive* archive);
  virtual ~AssemblerWorker();

  void run() noexcept override;

 private:
  const FolderParams& params_;
  MetaStorage* meta_storage_;
  ChunkStorage* chunk_storage_;
  PathNormalizer* path_normalizer_;
  Archive* archive_;

  SignedMeta smeta_;
  const Meta& meta_;

  QByteArray normpath_;
  QString denormpath_;

  bool assemble_deleted();
  bool assemble_symlink();
  bool assemble_directory();
  bool assemble_file();

  void apply_attrib();

  QByteArray get_chunk_pt(const blob& ct_hash) const;
};

}  // namespace librevault
