#pragma once
#include <QDateTime>
#include <QReadWriteLock>
#include <QStringList>

namespace librevault {

class FolderParams;
class PathNormalizer;

class IgnoreList {
 public:
  IgnoreList(const FolderParams& params, PathNormalizer& path_normalizer);

  bool isIgnored(QByteArray normpath);

 private:
  const FolderParams& params_;
  PathNormalizer& path_normalizer_;
  QStringList filters_wildcard_;

  QReadWriteLock ignorelist_mtx;
  QDateTime last_rebuild_;

  void lazyRebuildIgnores();
  void rebuildIgnores();  // not thread-safe!
  void parseLine(QString prefix, QString line);
  void addIgnorePattern(QString pattern, bool can_be_dir = true);
};

}  // namespace librevault
