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

  AssemblerWorker(const SignedMeta& smeta, const FolderParams& params, MetaStorage* meta_storage,
                  ChunkStorage* chunk_storage, PathNormalizer* path_normalizer, Archive* archive);
  ~AssemblerWorker() override;

  void run() noexcept override;

 private:
  const FolderParams& params_;
  MetaStorage* meta_storage_;
  ChunkStorage* chunk_storage_;
  PathNormalizer* path_normalizer_;
  Archive* archive_;

  Meta meta_;

  QByteArray normpath_;
  QString denormpath_;

  bool assemble_deleted();
  bool assemble_symlink();
  bool assemble_directory();
  bool assemble_file();

  void apply_attrib();

  QByteArray get_chunk_pt(const QByteArray& ct_hash) const;
};

}  // namespace librevault
