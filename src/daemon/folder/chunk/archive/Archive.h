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
//#include "util/fs.h"
#include <QObject>
#include <boost/filesystem/path.hpp>

#include "util/log.h"

namespace librevault {

struct FolderParams;
class MetaStorage;
class PathNormalizer;

struct ArchiveStrategy : public QObject {
  Q_OBJECT
 public:
  virtual void archive(const QString& denormpath) = 0;

 protected:
  ArchiveStrategy(QObject* parent) : QObject(parent) {}
};

class Archive : public QObject {
  Q_OBJECT
  LOG_SCOPE("Archive");

 public:
  Archive(const FolderParams& params, MetaStorage* meta_storage, PathNormalizer* path_normalizer, QObject* parent);

  bool archive(const QString& denormpath);

 private:
  MetaStorage* meta_storage_;
  PathNormalizer* path_normalizer_;

  ArchiveStrategy* archive_strategy_;
};

}  // namespace librevault
