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
#include <memory>

#include "Meta.h"
#include "util/log.h"

namespace librevault {

struct FolderParams;
class Secret;
class MetaStorage;
class PathNormalizer;

class OpenStorage : public QObject {
  Q_OBJECT
  LOG_SCOPE("OpenStorage");

 public:
  OpenStorage(const FolderParams& params, MetaStorage* meta_storage, PathNormalizer* path_normalizer, QObject* parent);

  bool have_chunk(const QByteArray& ct_hash) const noexcept;
  QByteArray get_chunk(const QByteArray& ct_hash) const;

 private:
  const FolderParams& params_;
  MetaStorage* meta_storage_;
  PathNormalizer* path_normalizer_;

  [[nodiscard]] inline bool verifyChunk(const QByteArray& ct_hash, const QByteArray& chunk_pt,
                                        Meta::StrongHashType strong_hash_type) const {
    return ct_hash == Meta::Chunk::computeStrongHash(chunk_pt, strong_hash_type);
  }
};

}  // namespace librevault
