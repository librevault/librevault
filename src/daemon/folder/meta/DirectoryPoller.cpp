#include "DirectoryPoller.h"

#include <QDirIterator>

#include "IndexerQueue.h"
#include "MetaStorage.h"
#include "control/FolderParams.h"
#include "folder/IgnoreList.h"
#include "folder/PathNormalizer.h"

namespace librevault {

DirectoryPoller::DirectoryPoller(const FolderParams& params, IgnoreList* ignore_list, PathNormalizer* path_normalizer,
                                 MetaStorage* parent)
    : QObject(parent),
      params_(params),
      meta_storage_(parent),
      ignore_list_(ignore_list),
      path_normalizer_(path_normalizer) {
  polling_timer_ = new QTimer(this);
  polling_timer_->setInterval(
      std::chrono::duration_cast<std::chrono::milliseconds>(params_.full_rescan_interval).count());
  polling_timer_->setTimerType(Qt::VeryCoarseTimer);

  connect(polling_timer_, &QTimer::timeout, this, &DirectoryPoller::addPathsToQueue);
}

DirectoryPoller::~DirectoryPoller() {}

void DirectoryPoller::setEnabled(bool enabled) {
  if (enabled) {
    QTimer::singleShot(0, this, &DirectoryPoller::addPathsToQueue);
    polling_timer_->start();
  } else
    polling_timer_->stop();
}

QList<QString> DirectoryPoller::getReindexList() {
  QSet<QString> file_list;

  // Files present in the file system
  qDebug() << params_.path;
  QDirIterator dir_it(params_.path, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                      params_.preserve_symlinks ? (QDirIterator::Subdirectories)
                                                : (QDirIterator::Subdirectories | QDirIterator::FollowSymlinks));
  while (dir_it.hasNext()) {
    QString abspath = dir_it.next();
    QByteArray normpath = path_normalizer_->normalizePath(abspath);

    if (!ignore_list_->isIgnored(normpath)) file_list.insert(abspath);
  }

  // Prevent incomplete (not assembled, partially-downloaded, whatever) from periodical scans.
  // They can still be indexed by monitor, though.
  for (auto& smeta : meta_storage_->getIncompleteMeta()) {
    QString denormpath = path_normalizer_->denormalizePath(smeta.meta().path(params_.secret));
    file_list.remove(denormpath);
  }

  // Files present in index (files added from here will be marked as DELETED)
  for (auto& smeta : meta_storage_->getExistingMeta()) {
    QByteArray normpath = smeta.meta().path(params_.secret);
    QString denormpath = path_normalizer_->denormalizePath(normpath);

    if (!ignore_list_->isIgnored(normpath)) file_list.insert(denormpath);
  }

  return file_list.values();
}

void DirectoryPoller::addPathsToQueue() {
  LOGD("Performing full directory rescan");

  for (QString denormpath : getReindexList()) {
    emit newPath(denormpath);
  }
}

}  // namespace librevault
