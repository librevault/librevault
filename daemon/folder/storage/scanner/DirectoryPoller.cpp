/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "DirectoryPoller.h"
#include "folder/FolderGroup.h"
#include "folder/IgnoreList.h"
#include "folder/storage/Index.h"
#include <path_normalizer.h>
#include <QDirIterator>

namespace librevault {

Q_LOGGING_CATEGORY(log_poller, "folder.storage.poller")

DirectoryPoller::DirectoryPoller(IgnoreList* ignore_list, Index* index, FolderGroup* parent)
    : QObject(parent), fgroup_(parent), index_(index), ignore_list_(ignore_list) {
  polling_timer_ = new QTimer(this);
  polling_timer_->setInterval(fgroup_->params().full_rescan_interval);
  polling_timer_->setTimerType(Qt::VeryCoarseTimer);

  connect(polling_timer_, &QTimer::timeout, this, &DirectoryPoller::addPathsToQueue);
}

DirectoryPoller::~DirectoryPoller() = default;

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
  auto filter = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System;
  auto flags = fgroup_->params().preserve_symlinks
               ? (QDirIterator::Subdirectories)
               : (QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
  QDirIterator dir_it(fgroup_->params().path, filter, flags);
  while (dir_it.hasNext()) {
    QString abspath = dir_it.next();
    QByteArray normpath = metakit::normalizePath(abspath, fgroup_->params().path);

    if (!ignore_list_->isIgnored(normpath)) file_list.insert(abspath);
  }

  // Prevent incomplete (not assembled, partially-downloaded, whatever) from periodical scans.
  // They can still be indexed by monitor, though.
  for (auto& smeta : index_->getIncompleteMeta()) {
    QString denormpath = metakit::absolutizePath(
        smeta.metaInfo().path().plaintext(fgroup_->params().secret.encryptionKey()), fgroup_->params().path);
    file_list.remove(denormpath);
  }

  // Files present in index (files added from here will be marked as DELETED)
  for (auto& smeta : index_->getExistingMeta()) {
    QByteArray normpath = smeta.metaInfo().path().plaintext(fgroup_->params().secret.encryptionKey());
    QString denormpath = metakit::absolutizePath(normpath, fgroup_->params().path);

    if (!ignore_list_->isIgnored(normpath)) file_list.insert(denormpath);
  }

  return file_list.values();
}

void DirectoryPoller::addPathsToQueue() {
  qCDebug(log_poller) << "Performing full directory rescan";

  for (const auto& denormpath : getReindexList()) emit newPath(denormpath);
}

} /* namespace librevault */
