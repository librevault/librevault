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
