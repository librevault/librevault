#include "OpenStorage.h"

#include "control/FolderParams.h"
#include "folder/PathNormalizer.h"
#include "folder/chunk/ChunkStorage.h"
#include "folder/meta/MetaStorage.h"

namespace librevault {

OpenStorage::OpenStorage(const FolderParams& params, MetaStorage* meta_storage, PathNormalizer* path_normalizer,
                         QObject* parent)
  : QObject(parent), params_(params), meta_storage_(meta_storage), path_normalizer_(path_normalizer) {}

bool OpenStorage::have_chunk(const QByteArray& ct_hash) const noexcept {
  return meta_storage_->isChunkAssembled(ct_hash);
}

QByteArray OpenStorage::get_chunk(const QByteArray& ct_hash) const {
  LOGD("get_chunk(" << ct_hash.toHex() << ")");

  for (auto& smeta : meta_storage_->containingChunk(ct_hash)) {
    // Search for chunk offset and index
    uint64_t offset = 0;
    int chunk_idx = 0;
    for (auto& chunk : smeta.meta().chunks()) {
      if (chunk.ct_hash == ct_hash) break;
      offset += chunk.size;
      chunk_idx++;
    }
    if (chunk_idx > smeta.meta().chunks().size()) {
      LOGW("Chunk not found in meta (index is inconsistent)!");
      continue;
    }

    // Found chunk & offset
    auto chunk = smeta.meta().chunks().at(chunk_idx);

    QFile f(path_normalizer_->denormalizePath(smeta.meta().path(params_.secret)));
    if (!f.open(QIODevice::ReadOnly)) {
      LOGW("Opening file failed!");
      continue;
    }
    if (!f.seek(offset)) {
      LOGW("Seek failed!");
      continue;
    }

    QByteArray chunk_pt = f.read(chunk.size);

    if (chunk_pt.size() != int(chunk.size)) {
      LOGW("File read failed! got=" << chunk_pt.size() << "needed=" << chunk.size);
      continue;
    }

    auto chunk_ct = Meta::Chunk::encrypt(chunk_pt, params_.secret.get_Encryption_Key(), chunk.iv);

    // Check
    if (verifyChunk(ct_hash, chunk_ct, smeta.meta().strong_hash_type())) {
      return chunk_ct;
    } else {
      LOGW("Chunk verification failed!");
    }
  }
  throw ChunkStorage::ChunkNotFound();
}

}  // namespace librevault