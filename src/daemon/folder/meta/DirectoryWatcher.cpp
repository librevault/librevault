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

  auto native_path = QDir::toNativeSeparators(params.path);

  qCDebug(log_watcher) << "DirectoryWatcher:" << native_path;

  addDirectory(native_path, true);

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
  qCDebug(log_watcher) << "handlePathEvent:" << path;

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

  QDirIterator it(path, QDir::AllEntries | QDir::NoDotAndDotDot,
    recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
  while (it.hasNext()) {
    QString new_path = it.next();
    if (!ignore_list_->isIgnored(path_normalizer_->normalizePath(new_path))) paths += new_path;
  }

  paths << path;

  watcher_->addPaths(paths);

  qCDebug(log_watcher) << paths;
}

}  // namespace librevault
