#pragma once
#include "Archive.h"

namespace librevault {

class TimestampArchive : public ArchiveStrategy {
  Q_OBJECT
 public:
  TimestampArchive(const FolderParams& params, PathNormalizer* path_normalizer, QObject* parent);
  void archive(const QString& denormpath) override;

 private:
  const FolderParams& params_;
  PathNormalizer* path_normalizer_;

  QString archive_path_;
};

}  // namespace librevault
