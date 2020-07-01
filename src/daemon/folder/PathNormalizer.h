#pragma once
#include <QString>

namespace librevault {

class FolderParams;

class PathNormalizer {
 public:
  PathNormalizer(const FolderParams& params);

  QByteArray normalizePath(const QString& abspath);
  QString denormalizePath(const QByteArray& normpath);

 private:
  const FolderParams& params_;
};

}  // namespace librevault
