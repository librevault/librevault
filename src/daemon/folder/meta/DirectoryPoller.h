#pragma once
#include <QTimer>

#include "Meta.h"
#include "util/log.h"

namespace librevault {

class FolderParams;
class IgnoreList;
class MetaStorage;
class PathNormalizer;

class DirectoryPoller : public QObject {
  Q_OBJECT
  LOG_SCOPE("DirectoryPoller");
 signals:
  void newPath(QString denormpath);

 public:
  DirectoryPoller(const FolderParams& params, IgnoreList* ignore_list, PathNormalizer* path_normalizer,
                  MetaStorage* parent);
  virtual ~DirectoryPoller();

 public slots:
  void setEnabled(bool enabled);

 private:
  const FolderParams& params_;
  MetaStorage* meta_storage_;
  IgnoreList* ignore_list_;
  PathNormalizer* path_normalizer_;

  QTimer* polling_timer_;

  QList<QString> getReindexList();

  void addPathsToQueue();
};

}  // namespace librevault
