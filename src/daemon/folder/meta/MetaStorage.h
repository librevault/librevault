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
#include <QObject>

#include "SignedMeta.h"

namespace librevault {

class DirectoryPoller;
class DirectoryWatcher;
struct FolderParams;
class IgnoreList;
class Index;
class IndexerQueue;
class PathNormalizer;
class StateCollector;

class MetaStorage : public QObject {
  Q_OBJECT
 signals:
  void metaAdded(SignedMeta meta);
  void metaAddedExternal(SignedMeta meta);

 public:
  struct MetaNotFound : public std::runtime_error {
    MetaNotFound() : std::runtime_error("Requested Meta not found") {}
  };

  MetaStorage(const FolderParams& params, IgnoreList* ignore_list, PathNormalizer* path_normalizer,
              StateCollector* state_collector, QObject* parent);
  virtual ~MetaStorage();

  bool haveMeta(const Meta::PathRevision& path_revision) noexcept;
  SignedMeta getMeta(const Meta::PathRevision& path_revision);
  SignedMeta getMeta(const QByteArray& path_id);
  QList<SignedMeta> getMeta();
  QList<SignedMeta> getExistingMeta();
  QList<SignedMeta> getIncompleteMeta();
  void putMeta(const SignedMeta& signed_meta, bool fully_assembled = false);
  QList<SignedMeta> containingChunk(const QByteArray& ct_hash);
  QPair<quint32, QByteArray> getChunkSizeIv(const QByteArray& ct_hash);

  // Assembled index
  void markAssembled(const QByteArray& path_id);
  bool isChunkAssembled(const QByteArray& ct_hash);

  bool putAllowed(const Meta::PathRevision& path_revision) noexcept;

  void prepareAssemble(QByteArray normpath, Meta::Type type, bool with_removal = false);

 private:
  Index* index_;
  IndexerQueue* indexer_;
  DirectoryPoller* poller_;
  DirectoryWatcher* watcher_;
};

}  // namespace librevault
