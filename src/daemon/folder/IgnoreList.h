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
