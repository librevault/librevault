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
#include "OpenStorage.h"

#include "control/FolderParams.h"
#include "folder/PathNormalizer.h"
#include "folder/chunk/ChunkStorage.h"
#include "folder/meta/MetaStorage.h"
#include "util/readable.h"

namespace librevault {

OpenStorage::OpenStorage(const FolderParams& params, MetaStorage* meta_storage, PathNormalizer* path_normalizer,
                         QObject* parent)
    : QObject(parent), params_(params), meta_storage_(meta_storage), path_normalizer_(path_normalizer) {}

bool OpenStorage::have_chunk(const QByteArray& ct_hash) const noexcept { return meta_storage_->isChunkAssembled(ct_hash); }

QByteArray OpenStorage::get_chunk(const QByteArray& ct_hash) const {
  LOGD("get_chunk(" << ct_hash_readable(ct_hash) << ")");

  for (auto& smeta : meta_storage_->containingChunk(ct_hash)) {
    // Search for chunk offset and index
    uint64_t offset = 0;
    int chunk_idx = 0;
    for (auto& chunk : smeta.meta().chunks()) {
      if (chunk.ct_hash == ct_hash) break;
      offset += chunk.size;
      chunk_idx++;
    }
    if (chunk_idx > smeta.meta().chunks().size()) continue;

    // Found chunk & offset
    auto chunk = smeta.meta().chunks().at(chunk_idx);
    blob chunk_pt = blob(chunk.size);

    QFile f(path_normalizer_->denormalizePath(smeta.meta().path(params_.secret)));
    if (!f.open(QIODevice::ReadOnly)) continue;
    if (!f.seek(offset)) continue;
    if (f.read(reinterpret_cast<char*>(chunk_pt.data()), chunk.size) != chunk.size) continue;

    auto chunk_ct = Meta::Chunk::encrypt(conv_bytearray(chunk_pt), params_.secret.get_Encryption_Key(), chunk.iv);

    // Check
    if (verifyChunk(ct_hash, chunk_ct, smeta.meta().strong_hash_type())) return chunk_ct;
  }
  throw ChunkStorage::ChunkNotFound();
}

}  // namespace librevault
