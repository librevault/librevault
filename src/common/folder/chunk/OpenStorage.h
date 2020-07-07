#pragma once
#include <QObject>
#include <memory>

#include "Meta.h"
#include "util/blob.h"
#include "util/log.h"

namespace librevault {

class FolderParams;
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

  [[nodiscard]] static bool verifyChunk(const QByteArray& ct_hash, const QByteArray& chunk_pt,
                                        Meta::StrongHashType strong_hash_type) {
    return ct_hash == Meta::Chunk::computeStrongHash(chunk_pt, strong_hash_type);
  }
};

}  // namespace librevault
