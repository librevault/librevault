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
#include "Downloader.h"

#include <QLoggingCategory>
#include <boost/range/adaptor/map.hpp>

#include "control/Config.h"
#include "control/FolderParams.h"
#include "folder/meta/MetaStorage.h"
#include "util/readable.h"

namespace librevault {

Q_LOGGING_CATEGORY(log_downloader, "folder.downloader")

DownloadChunk::DownloadChunk(const FolderParams& params, const QByteArray& ct_hash, quint32 size)
    : builder(params.system_path, ct_hash, size), ct_hash(ct_hash) {}

AvailabilityMap<uint32_t> DownloadChunk::requestMap() {
  AvailabilityMap<uint32_t> request_map = builder.file_map();
  for (auto& request : requests.values()) request_map.insert({request.offset, request.size});
  return request_map;
}

Downloader::Downloader(const FolderParams& params, MetaStorage* meta_storage, QObject* parent)
    : QObject(parent), params_(params), meta_storage_(meta_storage) {
  LOGFUNC();
  maintain_timer_ = new QTimer(this);
  connect(maintain_timer_, &QTimer::timeout, this, &Downloader::maintainRequests);
  maintain_timer_->setInterval(Config::get()->getGlobal("p2p_request_timeout").toInt() * 1000);
  maintain_timer_->setTimerType(Qt::VeryCoarseTimer);
  maintain_timer_->start();
}

Downloader::~Downloader() = default;

void Downloader::notifyLocalMeta(const SignedMeta& smeta, const bitfield_type& bitfield) {
  SCOPELOG(log_downloader);

  Q_ASSERT((size_t)bitfield.size() == (size_t)smeta.meta().chunks().size());

  QVector<QByteArray> incomplete_chunks;
  incomplete_chunks.reserve(smeta.meta().chunks().size());

  bool have_complete = false;
  bool have_incomplete = false;

  for (int chunk_idx = 0; chunk_idx < smeta.meta().chunks().size(); chunk_idx++) {
    auto& meta_chunk = smeta.meta().chunks().at(chunk_idx);

    if (bitfield[chunk_idx]) {
      have_complete = true;  // We have chunk, remove from missing
      // Do not mark connected chunks as clustered, because they will be marked inside the loop below.
      removeChunk(meta_chunk.ct_hash);
    } else {
      have_incomplete = true;  // We haven't this chunk, we need to download it
      addChunk(meta_chunk.ct_hash, meta_chunk.size);
      incomplete_chunks += meta_chunk.ct_hash;
    }
  }

  if (have_complete && have_incomplete)
    for (const QByteArray& ct_hash : getMetaCluster(incomplete_chunks)) download_queue_.markClustered(ct_hash);
}

void Downloader::addChunk(const QByteArray& ct_hash, quint32 size) {
  qCDebug(log_downloader) << "Added" << ct_hash.toHex() << "to download queue";

  uint32_t padded_size = size % 16 == 0 ? size : ((size / 16) + 1) * 16;

  DownloadChunkPtr chunk = std::make_shared<DownloadChunk>(params_, ct_hash, padded_size);
  down_chunks_.insert(ct_hash, chunk);

  download_queue_.addChunk(ct_hash);
}

void Downloader::removeChunk(const QByteArray& ct_hash) {
  if (down_chunks_.contains(ct_hash)) {
    download_queue_.removeChunk(ct_hash);
    down_chunks_.remove(ct_hash);

    qCDebug(log_downloader) << "Removed" << ct_hash.toHex() << "from download queue";
  }
}

void Downloader::notifyLocalChunk(const QByteArray& ct_hash) {
  SCOPELOG(log_downloader);

  removeChunk(ct_hash);

  // Mark all other chunks "clustered"
  for (const QByteArray& cluster_hash : getCluster(ct_hash)) download_queue_.markClustered(cluster_hash);
}

QSet<QByteArray> Downloader::getCluster(const QByteArray& ct_hash) {
  QSet<QByteArray> cluster;

  for (const SignedMeta& smeta : meta_storage_->containingChunk(ct_hash))
    for (auto& chunk : smeta.meta().chunks()) cluster += chunk.ct_hash;

  return cluster;
}

QSet<QByteArray> Downloader::getMetaCluster(const QVector<QByteArray>& ct_hashes) {
  QSet<QByteArray> cluster;
  for (const QByteArray& ct_hash : ct_hashes) cluster += getCluster(ct_hash);
  return cluster;
}

void Downloader::notifyRemoteMeta(RemoteFolder* remote, const Meta::PathRevision& revision, bitfield_type bitfield) {
  SCOPELOG(log_downloader);
  try {
    auto chunks = meta_storage_->getMeta(revision).meta().chunks();
    bitfield.resize(chunks.size(),
                    0);  // Because, incoming bitfield size is packed into octets, so it's size != chunk list size;
    for (int chunk_idx = 0; chunk_idx < chunks.size(); chunk_idx++)
      if (bitfield[chunk_idx]) notifyRemoteChunk(remote, chunks[chunk_idx].ct_hash);
  } catch (const MetaStorage::MetaNotFound&) {
    qCDebug(log_downloader) << "Expired Meta";
    // Well, remote node notifies us about expired meta. It was not requested by us OR another peer sent us newer meta,
    // so this had been expired. Nevertheless, ignore this notification.
  }
}
void Downloader::notifyRemoteChunk(RemoteFolder* remote, const QByteArray& ct_hash) {
  SCOPELOG(log_downloader);
  DownloadChunkPtr chunk = down_chunks_.value(ct_hash);
  if (!chunk) return;

  chunk->owned_by.insert(remote, remote->get_interest_guard());
  download_queue_.setRemotesCount(ct_hash, chunk->owned_by.size());

  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::handleChoke(RemoteFolder* remote) {
  SCOPELOG(log_downloader);

  /* Remove requests to this node */
  for (const DownloadChunkPtr& missing_chunk : down_chunks_) missing_chunk->requests.remove(remote);

  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::handleUnchoke(RemoteFolder* remote) {
  SCOPELOG(log_downloader);
  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::putBlock(const QByteArray& ct_hash, uint32_t offset, const QByteArray& data, RemoteFolder* from) {
  SCOPELOG(log_downloader);
  auto missing_chunk = down_chunks_.value(ct_hash);
  if (!missing_chunk) {
    qCDebug(log_downloader) << "Chunk not found:" << ct_hash.toHex() << "offset: " << offset;
    return;
  }

  QList<QPair<QByteArray, QFile*>> downloaded_chunks;

  QMutableHashIterator<RemoteFolder*, DownloadChunk::BlockRequest> request_it(missing_chunk->requests);
  while (request_it.hasNext()) {
    request_it.next();

    if (request_it.value().offset != offset) continue;     // Chunk position incorrect
    if (request_it.value().size != data.size()) continue;  // Chunk size incorrect
    if (request_it.key() != from)
      continue;  // Requested node != replied. Well, it isn't critical, but will be useful to ban "fake" peers

    request_it.remove();

    missing_chunk->builder.put_block(offset, data);
    if (missing_chunk->builder.complete()) {
      QFile* chunk_f = missing_chunk->builder.release_chunk();
      chunk_f->setParent(this);

      downloaded_chunks += qMakePair(ct_hash, chunk_f);
    }  // TODO: catch "invalid hash" exception here
  }

  for (const QPair<QByteArray, QFile*>& chunk : downloaded_chunks) emit chunkDownloaded(chunk.first, chunk.second);

  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::trackRemote(RemoteFolder* remote) {
  remotes_.insert(remote);
  download_queue_.setRemotesCount(remotes_.size());
}

void Downloader::untrackRemote(RemoteFolder* remote) {
  SCOPELOG(log_downloader);

  if (!remotes_.contains(remote)) return;

  for (const DownloadChunkPtr& missing_chunk : down_chunks_.values()) {
    missing_chunk->requests.remove(remote);
    missing_chunk->owned_by.remove(remote);
    download_queue_.setRemotesCount(missing_chunk->ct_hash, missing_chunk->owned_by.size());
  }
  remotes_.remove(remote);
  download_queue_.setRemotesCount(remotes_.size());
}

void Downloader::maintainRequests() {
  SCOPELOG(log_downloader);

  // Prune old requests by timeout
  {
    auto request_timeout = std::chrono::seconds(Config::get()->getGlobal("p2p_request_timeout").toUInt());
    for (const DownloadChunkPtr& missing_chunk : down_chunks_.values()) {
      QMutableHashIterator<RemoteFolder*, DownloadChunk::BlockRequest> request_it(missing_chunk->requests);
      while (request_it.hasNext())
        if (request_it.next().value().started + request_timeout < std::chrono::steady_clock::now()) {
          qCDebug(log_downloader) << "Request timed out: offset=" << request_it.value().offset << "size=" << request_it.value().size;
          request_it.remove();
        }
    }
  }

  // Make new requests
  for (size_t i = countRequests(); i < Config::get()->getGlobal("p2p_download_slots").toUInt(); i++) {
    bool requested = requestOne();
    if (!requested) break;
  }
}

bool Downloader::requestOne() {
  SCOPELOG(log_downloader);
  // Try to choose chunk to request
  for (const QByteArray& ct_hash : download_queue_.chunks()) {
    // Try to choose a remote to request this block from
    auto remote = nodeForRequest(ct_hash);
    if (!remote){
      qCDebug(log_downloader) << "Node not found for request";
      continue;
    }

    DownloadChunkPtr chunk = down_chunks_.value(ct_hash);

    // Rebuild request map to determine, which block to download now.
    AvailabilityMap<uint32_t> request_map = chunk->requestMap();

    // Request, actually
    if (!request_map.full()) {
      DownloadChunk::BlockRequest request;
      request.offset = request_map.begin()->first;
      request.size =
          std::min(request_map.begin()->second, uint32_t(Config::get()->getGlobal("p2p_block_size").toUInt()));
      request.started = std::chrono::steady_clock::now();

      remote->request_block(ct_hash, request.offset, request.size);
      chunk->requests.insert(remote, request);
      return true;
    }else{
      qCDebug(log_downloader) << "Request map if full!";
    }
  }
  return false;
}

RemoteFolder* Downloader::nodeForRequest(const QByteArray& ct_hash) {
  DownloadChunkPtr chunk = down_chunks_.value(ct_hash);
  if (!chunk) return nullptr;

  for (RemoteFolder* owner_remote : chunk->owned_by.keys())
    if (owner_remote->ready() && !owner_remote->peer_choking())
      return owner_remote;  // TODO: implement more smart peer selection algorithm, based on peer weights.

  return nullptr;
}

size_t Downloader::countRequests() const {
  size_t requests = 0;
  for (const DownloadChunkPtr& chunk : down_chunks_.values()) requests += chunk->requests.size();
  return requests;
}

}  // namespace librevault
