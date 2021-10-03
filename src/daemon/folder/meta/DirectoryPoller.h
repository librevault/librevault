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
