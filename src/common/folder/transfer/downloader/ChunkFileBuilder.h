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
