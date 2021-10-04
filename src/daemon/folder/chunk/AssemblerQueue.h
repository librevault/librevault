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
#include <QThreadPool>
#include <QTimer>

#include "SignedMeta.h"

namespace librevault {

class PathNormalizer;

class Archive;
class MetaStorage;
struct FolderParams;
class ChunkStorage;
class Secret;

class AssemblerQueue : public QObject {
  Q_OBJECT
 signals:
  void aboutToStop();

  void startedAssemble();
  void finishedAssemble();

 public:
  AssemblerQueue(const FolderParams& params, MetaStorage* meta_storage, ChunkStorage* chunk_storage,
                 PathNormalizer* path_normalizer, Archive* archive, QObject* parent);
  virtual ~AssemblerQueue();

 public slots:
  void addAssemble(SignedMeta smeta);

 private:
  const FolderParams& params_;
  MetaStorage* meta_storage_;
  ChunkStorage* chunk_storage_;
  PathNormalizer* path_normalizer_;
  Archive* archive_;

  QThreadPool* threadpool_;

  void periodicAssembleOperation();
  QTimer* assemble_timer_;
};

}  // namespace librevault
