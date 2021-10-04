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
#include <QMap>
#include <QString>
#include <QThreadPool>

#include "SignedMeta.h"

namespace librevault {

struct FolderParams;
class MetaStorage;
class IgnoreList;
class PathNormalizer;
class StateCollector;
class IndexerWorker;
class IndexerQueue : public QObject {
  Q_OBJECT
 signals:
  void aboutToStop();

  void startedIndexing();
  void finishedIndexing();

 public:
  IndexerQueue(const FolderParams& params, IgnoreList* ignore_list, PathNormalizer* path_normalizer,
               StateCollector* state_collector, QObject* parent);
  virtual ~IndexerQueue();

 public slots:
  void addIndexing(QString abspath);

 private:
  const FolderParams& params_;
  MetaStorage* meta_storage_;
  IgnoreList* ignore_list_;
  PathNormalizer* path_normalizer_;
  StateCollector* state_collector_;

  QThreadPool* threadpool_;

  const Secret& secret_;

  QMap<QString, IndexerWorker*> tasks_;

 private slots:
  void metaCreated(SignedMeta smeta);
  void metaFailed(QString error_string);
};

}  // namespace librevault
