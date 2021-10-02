#pragma once
#include <QList>
#include <QTimer>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <chrono>

#include "folder/RemoteFolder.h"
#include "folder/transfer/downloader/ChunkFileBuilder.h"
#include "folder/transfer/downloader/WeightedChunkQueue.h"
#include "util/AvailabilityMap.h"
#include "util/log.h"

#define CLUSTERED_COEFFICIENT 10.0f
#define IMMEDIATE_COEFFICIENT 20.0f
#define RARITY_COEFFICIENT 25.0f

namespace librevault {

class FolderParams;
class MetaStorage;
class ChunkStorage;

struct DownloadChunk : boost::noncopyable {
  DownloadChunk(const FolderParams& params, const QByteArray& ct_hash, quint32 size);

  ChunkFileBuilder builder;

  AvailabilityMap<uint32_t> requestMap();

  /* Request-oriented functions */
  struct BlockRequest {
    uint32_t offset = 0;
    uint32_t size = 0;
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
  void notifyLocalMeta(const SignedMeta& smeta, const QBitArray& bitfield);
  void notifyLocalChunk(const QByteArray& ct_hash);

  void notifyRemoteMeta(RemoteFolder* remote, const Meta::PathRevision& revision, QBitArray bitfield);
  void notifyRemoteChunk(RemoteFolder* remote, const QByteArray& ct_hash);

  void handleChoke(RemoteFolder* remote);
  void handleUnchoke(RemoteFolder* remote);

  void putBlock(const QByteArray& ct_hash, uint32_t offset, const QByteArray& data, RemoteFolder* from);

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
  RemoteFolder* nodeForRequest(const QByteArray& ct_hash);

  void addChunk(const QByteArray& ct_hash, quint32 size);
  void removeChunk(const QByteArray& ct_hash);

  /* Node management */
  QSet<RemoteFolder*> remotes_;

  QSet<QByteArray> getCluster(const QByteArray& ct_hash);
  QSet<QByteArray> getMetaCluster(const QVector<QByteArray>& ct_hashes);
};

}  // namespace librevault
