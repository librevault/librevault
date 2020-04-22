/* Copyright (C) 2020 Alexander Shishenko <alex@shishenko.com>
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
#include "DirectoryWatcher.h"

#include <QTimer>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>

#include "control/FolderParams.h"
#include "folder/IgnoreList.h"
#include "folder/PathNormalizer.h"

namespace librevault {

Q_LOGGING_CATEGORY(log_watcher, "folder.watcher")

DirectoryWatcher::DirectoryWatcher(const FolderParams& params, IgnoreList* ignore_list, PathNormalizer* path_normalizer,
                                   QObject* parent)
    : QObject(parent), params_(params), ignore_list_(ignore_list), path_normalizer_(path_normalizer) {
  watcher_ = new QFileSystemWatcher(this);

  addDirectory(params.path, true);

  connect(watcher_, &QFileSystemWatcher::directoryChanged, this,
          [this](const QString& path) { addDirectory(path, false); });
  connect(watcher_, &QFileSystemWatcher::directoryChanged, this, &DirectoryWatcher::handlePathEvent);
  connect(watcher_, &QFileSystemWatcher::fileChanged, this, &DirectoryWatcher::handlePathEvent);
}

DirectoryWatcher::~DirectoryWatcher() = default;

void DirectoryWatcher::prepareAssemble(const QByteArray& normpath, Meta::Type type, bool with_removal) {
  unsigned skip_events = 0;
  if (with_removal || type == Meta::Type::DELETED) skip_events++;

  switch (type) {
    case Meta::FILE:
      skip_events += 2;  // RENAMED (NEW NAME), MODIFIED
      break;
    case Meta::DIRECTORY:
      skip_events += 1;  // ADDED
      break;
    default:;
  }

  for (unsigned i = 0; i < skip_events; i++) prepared_assemble_.insert(normpath);
}

void DirectoryWatcher::handlePathEvent(const QString& path) {
//  qCDebug(log_watcher) << "path:" << path;

  QByteArray normpath = path_normalizer_->normalizePath(path);

  auto prepared_assemble_it = prepared_assemble_.find(normpath);
  if (prepared_assemble_it != prepared_assemble_.end()) {
    prepared_assemble_.erase(prepared_assemble_it);
    return;  // FIXME: "prepares" is a dirty hack. It must be EXTERMINATED!
  }

  if (!ignore_list_->isIgnored(normpath)) emit newPath(path);
}

void DirectoryWatcher::addDirectory(const QString& path, bool recursive) {
  QStringList paths;

  QDirIterator it(path, QDir::AllEntries | QDir::NoDotAndDotDot, recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
  while (it.hasNext()) {
    QString new_path = it.next();
    if (!ignore_list_->isIgnored(path_normalizer_->normalizePath(new_path))) paths += new_path;
  }

  watcher_->addPaths(paths);

  qCDebug(log_watcher) << paths;
}

}  // namespace librevault
