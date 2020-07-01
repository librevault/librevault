#include "ChunkStorage.h"

#include "AssemblerQueue.h"
#include "EncStorage.h"
#include "MemoryStorage.h"
#include "OpenStorage.h"
#include "control/FolderParams.h"
#include "folder/chunk/archive/Archive.h"
#include "folder/meta/MetaStorage.h"

namespace librevault {

ChunkStorage::ChunkStorage(const FolderParams& params, MetaStorage* meta_storage, PathNormalizer* path_normalizer,
                           QObject* parent)
    : QObject(parent), meta_storage_(meta_storage) {
  mem_storage = new MemoryStorage(this);
  enc_storage = new EncStorage(params, this);
  if (params.secret.get_type() <= Secret::Type::ReadOnly) {
    open_storage = new OpenStorage(params, meta_storage_, path_normalizer, this);
    archive = new Archive(params, meta_storage_, path_normalizer, this);
    file_assembler = new AssemblerQueue(params, meta_storage_, this, path_normalizer, archive, this);
  }

  connect(meta_storage_, &MetaStorage::metaAddedExternal, file_assembler, &AssemblerQueue::addAssemble);
}

bool ChunkStorage::have_chunk(const QByteArray& ct_hash) const noexcept {
  return mem_storage->have_chunk(ct_hash) || enc_storage->have_chunk(ct_hash) ||
         (open_storage && open_storage->have_chunk(ct_hash));
}

QByteArray ChunkStorage::get_chunk(const QByteArray& ct_hash) {
  try {
    // Cache hit
    return mem_storage->get_chunk(ct_hash);
  } catch (ChunkNotFound& e) {
    // Cache missed
    QByteArray chunk;
    try {
      chunk = enc_storage->get_chunk(ct_hash);
    } catch (ChunkNotFound& e) {
      if (open_storage)
        chunk = open_storage->get_chunk(ct_hash);
      else
        throw;
    }
    mem_storage->put_chunk(ct_hash, chunk);  // Put into cache
    return chunk;
  }
}

void ChunkStorage::put_chunk(const QByteArray& ct_hash, QFile* chunk_f) {
  enc_storage->put_chunk(ct_hash, chunk_f);
  for (auto& smeta : meta_storage_->containingChunk(ct_hash)) file_assembler->addAssemble(smeta);

  emit chunkAdded(ct_hash);
}

QBitArray ChunkStorage::make_bitfield(const Meta& meta) const noexcept {
  QBitArray bf;
  if (meta.meta_type() == meta.FILE) {
    bf.resize(meta.chunks().size());

    for (int bitfield_idx = 0; bitfield_idx < meta.chunks().size(); bitfield_idx++)
      if (have_chunk(meta.chunks().at(bitfield_idx).ct_hash)) bf[bitfield_idx] = true;
  }
  return bf;
}

void ChunkStorage::cleanup(const Meta& meta) {
  for (const auto& chunk : meta.chunks())
    if (open_storage->have_chunk(chunk.ct_hash)) enc_storage->remove_chunk(chunk.ct_hash);
}

}  // namespace librevault
