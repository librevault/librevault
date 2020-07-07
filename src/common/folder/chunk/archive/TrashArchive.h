#pragma once
#include "Archive.h"

namespace librevault {

class TrashArchive : public ArchiveStrategy {
  Q_OBJECT
 public:
  TrashArchive(const FolderParams& params, PathNormalizer* path_normalizer, QObject* parent);
  void archive(const QString& denormpath) override;

 private:
  const FolderParams& params_;
  PathNormalizer* path_normalizer_;

  QString archive_path_;

  void maintain_cleanup();
};

}  // namespace librevault
