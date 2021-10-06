#pragma once
#include <QtCore/QFileSystemWatcher>
#include <set>

#include "Meta.h"
#include "util/log.h"

namespace librevault {

Q_DECLARE_LOGGING_CATEGORY(log_watcher)

struct FolderParams;
class IgnoreList;
class PathNormalizer;

class DirectoryWatcher : public QObject {
Q_OBJECT
signals:
  void newPath(QString abspath);

public:
  DirectoryWatcher(const FolderParams& params, IgnoreList* ignore_list, PathNormalizer* path_normalizer,
                   QObject* parent);
  ~DirectoryWatcher() override;

  // A VERY DIRTY HACK
  void prepareAssemble(const QByteArray& normpath, Meta::Type type, bool with_removal = false);

private:
  const FolderParams& params_;
  IgnoreList* ignore_list_;
  PathNormalizer* path_normalizer_;
  QFileSystemWatcher* watcher_;

  std::multiset<QString> prepared_assemble_;

private slots:
  void handlePathEvent(const QString& path);
  void addDirectory(const QString& path, bool recursive);
};

}  // namespace librevault
