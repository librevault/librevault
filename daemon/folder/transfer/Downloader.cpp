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
#include "Downloader.h"

#include "control/Config.h"
#include "config/models.h"
#include "folder/storage/Index.h"
#include <ChunkInfo.h>
#include <QLoggingCategory>

namespace librevault {

Q_LOGGING_CATEGORY(log_downloader, "folder.transfer.downloader")

DownloadChunk::DownloadChunk(const models::FolderSettings& params, const QByteArray& ct_hash, quint32 size)
    : builder(params.effectiveSystemPath(), ct_hash, size), ct_hash(ct_hash) {}

Downloader::Downloader(const models::FolderSettings& params, Index* index, QObject* parent)
    : QObject(parent), params_(params), index_(index) {
  maintain_timer_ = new QTimer(this);
  connect(maintain_timer_, &QTimer::timeout, this, &Downloader::maintainRequests);
  maintain_timer_->setInterval(Config::get()->getGlobals().p2p_request_timeout * 1000);
  maintain_timer_->setTimerType(Qt::VeryCoarseTimer);
  maintain_timer_->start();
}

void Downloader::notifyLocalMeta(const SignedMeta& smeta, const QBitArray& bitfield) {
  Q_ASSERT(bitfield.size() == smeta.metaInfo().chunks().size());

  QList<QByteArray> incomplete_chunks;
  incomplete_chunks.reserve(smeta.metaInfo().chunks().size());

  bool have_complete = false;
  bool have_incomplete = false;

  auto chunks = smeta.metaInfo().chunks();
  for (int chunk_idx = 0; chunk_idx < chunks.size(); chunk_idx++) {
    auto& meta_chunk = chunks.at(chunk_idx);

    QByteArray ct_hash = meta_chunk.ctHash();

    if (bitfield.testBit(chunk_idx)) {
      // We have chunk, remove from missing
      have_complete = true;
      // Do not mark connected chunks as clustered, because they will be marked inside the loop
      // below.
      removeChunk(ct_hash);
    } else {
      // We haven't this chunk, we need to download it
      have_incomplete = true;
      addChunk(ct_hash, meta_chunk.size());
      incomplete_chunks << ct_hash;
    }
  }

  if (have_complete && have_incomplete)
    for (const QByteArray& ct_hash : getMetaCluster(incomplete_chunks))
      download_queue_.markClustered(ct_hash);
}

void Downloader::addChunk(const QByteArray& ct_hash, quint32 size) {
  qCDebug(log_downloader) << "Added" << ct_hash.toHex() << "to download queue";

  uint32_t padded_size = ((size / 16) + 1) * 16;

  DownloadChunkPtr chunk = std::make_shared<DownloadChunk>(params_, ct_hash, padded_size);
  down_chunks_.insert(ct_hash, chunk);

  download_queue_.addChunk(ct_hash);
}

void Downloader::removeChunk(const QByteArray& ct_hash) {
  if (down_chunks_.contains(ct_hash)) {
    request_tracker_.removeExisting(ct_hash);
    download_queue_.removeChunk(ct_hash);
    down_chunks_.remove(ct_hash);

    qCDebug(log_downloader) << "Removed" << ct_hash.toHex() << "from download queue";
  }
}

void Downloader::notifyLocalChunk(const QByteArray& ct_hash) {
  removeChunk(ct_hash);

  // Mark all other chunks "clustered"
  for (const QByteArray& cluster_hash : getCluster(ct_hash))
    download_queue_.markClustered(cluster_hash);
}

QSet<QByteArray> Downloader::getCluster(const QByteArray& ct_hash) {
  QSet<QByteArray> cluster;
  for (const SignedMeta& smeta : index_->containingChunk(ct_hash))
    for (auto& chunk : smeta.metaInfo().chunks()) cluster << chunk.ctHash();
  return cluster;
}

QSet<QByteArray> Downloader::getMetaCluster(const QList<QByteArray>& ct_hashes) {
  QSet<QByteArray> cluster;
  for (const QByteArray& ct_hash : ct_hashes) cluster += getCluster(ct_hash);
  return cluster;
}

void Downloader::notifyRemoteMeta(
    Peer* peer, const MetaInfo::PathRevision& revision, const QBitArray& bitfield) {
  qCDebug(log_downloader) << "Meta notification:" << peer << revision << bitfield;

  try {
    auto chunks = index_->getMeta(revision).metaInfo().chunks();
    if (chunks.size() != bitfield.size()) throw InconsistentMetaBetweenPeers();

    for (int chunk_idx = 0; chunk_idx < chunks.size(); chunk_idx++)
      if (bitfield.testBit(chunk_idx)) notifyRemoteChunk(peer, chunks[chunk_idx].ctHash());
  } catch (const Index::NoSuchMeta&) {
    qCDebug(log_downloader) << "Expired Meta";
  } catch (const InconsistentMetaBetweenPeers& e) {
    qCWarning(log_downloader) << e.what();
  }
}
void Downloader::notifyRemoteChunk(Peer* peer, const QByteArray& ct_hash) {
  qCDebug(log_downloader) << "Chunk notification:" << peer << ct_hash.toHex();

  DownloadChunkPtr chunk = down_chunks_.value(ct_hash);
  if (!chunk) return;

  chunk->owned_by.insert(peer, peer->getInterestGuard());
  download_queue_.setPeerCount(ct_hash, chunk->owned_by.size());

  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::handleChoke(Peer* peer) {
  request_tracker_.peerChoked(peer);
  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::handleUnchoke(Peer* peer) {
  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::putBlock(
    const QByteArray& ct_hash, uint32_t offset, const QByteArray& data, Peer* from) {
  if (request_tracker_.completeRequest(ct_hash, offset, data.size(), from)) {
    Q_ASSERT(down_chunks_.contains(ct_hash));
    ChunkFileBuilder& builder = down_chunks_[ct_hash]->builder;

    builder.putBlock(offset, data);
    if (builder.complete()) {
      QFile* downloaded_chunk_f = builder.releaseChunk();
      downloaded_chunk_f->setParent(nullptr);  // make it orphan
      emit chunkDownloaded(ct_hash, downloaded_chunk_f);
    }
  }

  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::trackPeer(Peer* peer) {
  Q_ASSERT(!peers_.contains(peer));

  peers_.insert(peer);
  download_queue_.setPeerCount(peers_.size());
}

void Downloader::untrackPeer(Peer* peer) {
  Q_ASSERT(peers_.contains(peer));

  request_tracker_.peerDisconnected(peer);

  for (DownloadChunkPtr missing_chunk : down_chunks_.values()) {
    missing_chunk->owned_by.remove(peer);
    download_queue_.setPeerCount(missing_chunk->ct_hash, missing_chunk->owned_by.size());
  }
  peers_.remove(peer);
  download_queue_.setPeerCount(peers_.size());
}

void Downloader::maintainRequests() {
  request_tracker_.removeExpired();     // Prune old requests by timeout
  while (request_tracker_.freeSlots())  // Make new requests
    if (!requestOne()) break;
}

bool Downloader::requestOne() {
  // Try to choose chunk to request
  for (const QByteArray& ct_hash : download_queue_.chunks()) {
    auto peer = peerForRequest(ct_hash);  // Try to choose a peer to request this block from
    if (!peer) continue;

    // Rebuild request map to determine, which block to download now.
    AvailabilityMap<quint32> request_map = down_chunks_[ct_hash]->builder.fileMap();
    for (const auto& request : request_tracker_.requestsForChunk(ct_hash))
      request_map.insert({request.offset, request.size});

    // Request, actually
    if (!request_map.full()) {
      request_tracker_.createRequest(
          ct_hash, request_map.begin()->first, request_map.begin()->second, peer);
      return true;
    }
  }
  return false;
}

Peer* Downloader::peerForRequest(const QByteArray& ct_hash) {
  DownloadChunkPtr chunk = down_chunks_.value(ct_hash);
  if (!chunk) return nullptr;

  for (Peer* peer : chunk->owned_by.keys())
    if (peer->isValid() && !peer->peerChoking())
      return peer;  // TODO: implement more smart peer selection algorithm, based on peer weights.

  return nullptr;
}

}  // namespace librevault
