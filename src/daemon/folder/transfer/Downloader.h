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
#pragma once
#include <QList>
#include <QTimer>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <chrono>

#include "downloader/ChunkFileBuilder.h"
#include "downloader/WeightedChunkQueue.h"
#include "folder/RemoteFolder.h"
#include "util/AvailabilityMap.h"
#include "util/blob.h"
#include "util/log.h"

#define CLUSTERED_COEFFICIENT 10.0f
#define IMMEDIATE_COEFFICIENT 20.0f
#define RARITY_COEFFICIENT 25.0f

namespace librevault {

class FolderParams;
class MetaStorage;
class ChunkStorage;

struct DownloadChunk : boost::noncopyable {
  DownloadChunk(const FolderParams& params, QByteArray ct_hash, quint32 size);

  ChunkFileBuilder builder;

  AvailabilityMap<uint32_t> requestMap();

  /* Request-oriented functions */
  struct BlockRequest {
    uint32_t offset;
    uint32_t size;
    std::chrono::steady_clock::time_point started;
  };
  QMultiHash<RemoteFolder*, BlockRequest> requests;
  QHash<RemoteFolder*, std::shared_ptr<RemoteFolder::InterestGuard>> owned_by;

  const QByteArray ct_hash;
};

using DownloadChunkPtr = std::shared_ptr<DownloadChunk>;

class Downloader : public QObject {
  Q_OBJECT
 signals:
  void chunkDownloaded(QByteArray ct_hash, QFile* chunk_f);

 public:
  Downloader(const FolderParams& params, MetaStorage* meta_storage, QObject* parent);
  ~Downloader();

 public slots:
  void notifyLocalMeta(const SignedMeta& smeta, const bitfield_type& bitfield);
  void notifyLocalChunk(const QByteArray& ct_hash);

  void notifyRemoteMeta(RemoteFolder* remote, const Meta::PathRevision& revision, bitfield_type bitfield);
  void notifyRemoteChunk(RemoteFolder* remote, const blob& ct_hash);

  void handleChoke(RemoteFolder* remote);
  void handleUnchoke(RemoteFolder* remote);

  void putBlock(const blob& ct_hash, uint32_t offset, const blob& data, RemoteFolder* from);

  void trackRemote(RemoteFolder* remote);
  void untrackRemote(RemoteFolder* remote);

 private:
  const FolderParams& params_;
  MetaStorage* meta_storage_;

  QHash<QByteArray, DownloadChunkPtr> down_chunks_;
  WeightedChunkQueue download_queue_;

  size_t countRequests() const;

  /* Request process */
  QTimer* maintain_timer_;

  void maintainRequests();
  bool requestOne();
  RemoteFolder* nodeForRequest(QByteArray ct_hash);

  void addChunk(const QByteArray& ct_hash, quint32 size);
  void removeChunk(const QByteArray& ct_hash);

  /* Node management */
  QSet<RemoteFolder*> remotes_;

  QSet<QByteArray> getCluster(const QByteArray& ct_hash);
  QSet<QByteArray> getMetaCluster(const QList<QByteArray>& ct_hashes);
};

} /* namespace librevault */
