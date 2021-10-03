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
 */
#include "IgnoreList.h"

#include <QDirIterator>
#include <QLoggingCategory>

#include "control/FolderParams.h"
#include "folder/PathNormalizer.h"

Q_LOGGING_CATEGORY(log_ignorelist, "folder.ignorelist")

namespace librevault {

IgnoreList::IgnoreList(const FolderParams& params, PathNormalizer& path_normalizer)
    : params_(params), path_normalizer_(path_normalizer), last_rebuild_(QDateTime::fromMSecsSinceEpoch(0)) {
  lazyRebuildIgnores();
}

bool IgnoreList::isIgnored(QByteArray normpath) {
  lazyRebuildIgnores();

  QReadLocker lk(&ignorelist_mtx);
  return QDir::match(filters_wildcard_, QString::fromUtf8(normpath));
}

void IgnoreList::lazyRebuildIgnores() {
  QWriteLocker lk(&ignorelist_mtx);
  if (last_rebuild_.msecsTo(QDateTime::currentDateTimeUtc()) > 10 * 1000) {
    rebuildIgnores();
    last_rebuild_ = QDateTime::currentDateTimeUtc();
  }
}

void IgnoreList::rebuildIgnores() {
  filters_wildcard_.clear();

  // System folder
  addIgnorePattern(".librevault");

  QDirIterator dir_it(params_.path, QStringList() << ".lvignore", QDir::Files | QDir::Hidden | QDir::Readable,
                      QDirIterator::Subdirectories);
  qCDebug(log_ignorelist) << "Rebuilding ignore list";
  while (dir_it.hasNext()) {
    QString ignorefile_path = dir_it.next();
    qCDebug(log_ignorelist) << "Found ignore file:" << ignorefile_path;

    // Compute "root" for current ignore file
    QString ignore_prefix = QString::fromUtf8(path_normalizer_.normalizePath(ignorefile_path));
    ignore_prefix.chop(QStringLiteral(".lvignore").size());
    qCDebug(log_ignorelist) << "Ignore file prefix:" << (ignore_prefix.isEmpty() ? "(root)" : ignore_prefix);

    // Read ignore file
    QFile ignorefile(ignorefile_path);
    if (ignorefile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream stream(&ignorefile);
      stream.setCodec("UTF-8");
      while (!stream.atEnd()) {
        parseLine(ignore_prefix, stream.readLine());
      }
    } else
      qCWarning(log_ignorelist) << "Could not open ignore file:" << ignorefile_path;
  }
}

void IgnoreList::parseLine(QString prefix, QString line) {
  if (line.size() == 0) return;
  if (line.left(1) == "#") return;                  // comment encountered
  if (line.left(2) == R"(\#)") line = line.mid(1);  // escape hash

  // This is not a comment
  if (!QDir::isRelativePath(line)) {
    qCWarning(log_ignorelist) << "Ignored path is not relative:" << line;
    return;
  }

  line = QDir::cleanPath(line);
  if (line.left(2) == "../" || line == "..") {
    qCWarning(log_ignorelist) << "Ignored path does not support \"..\"";
    return;
  }

  addIgnorePattern(prefix + line);
}

void IgnoreList::addIgnorePattern(QString pattern, bool can_be_dir) {
  filters_wildcard_ << pattern;
  qCDebug(log_ignorelist) << "Added ignore pattern:" << pattern;
  if (can_be_dir) {
    QString pattern_directory = pattern + "/*";  // If it is a directory, then ignore all files inside!
    filters_wildcard_ << pattern_directory;
    qCDebug(log_ignorelist) << "Added helper directory ignore pattern:" << pattern_directory;
  }
}

}  // namespace librevault
