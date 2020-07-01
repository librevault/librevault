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

  QBitArray make_bitfield(const Meta& meta) const noexcept;  // Bulk version of "have_chunk"

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
