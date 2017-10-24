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
#include "control/FolderParams.h"
#include "folder/storage/Index.h"
#include <ChunkInfo.h>
#include <QLoggingCategory>

namespace librevault {

Q_LOGGING_CATEGORY(log_downloader, "folder.transfer.downloader")

DownloadChunk::DownloadChunk(const FolderParams& params, QByteArray ct_hash, quint32 size)
    : builder(params.system_path, ct_hash, size), ct_hash(ct_hash) {}

AvailabilityMap<uint32_t> DownloadChunk::requestMap() {
  AvailabilityMap<uint32_t> request_map = builder.file_map();
  for (auto& request : requests.values()) request_map.insert({request.offset, request.size});
  return request_map;
}

Downloader::Downloader(const FolderParams& params, Index* index, QObject* parent)
    : QObject(parent), params_(params), index_(index) {
  maintain_timer_ = new QTimer(this);
  connect(maintain_timer_, &QTimer::timeout, this, &Downloader::maintainRequests);
  maintain_timer_->setInterval(Config::get()->getGlobal("p2p_request_timeout").toInt() * 1000);
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
      // Do not mark connected chunks as clustered, because they will be marked inside the loop below.
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

  uint32_t padded_size = size % 16 == 0 ? size : ((size / 16) + 1) * 16;  // TODO: pad always

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
  try {
    auto chunks = index_->getMeta(revision).metaInfo().chunks();
    for (int chunk_idx = 0; chunk_idx < chunks.size(); chunk_idx++)
      if (bitfield.testBit(chunk_idx)) notifyRemoteChunk(peer, chunks[chunk_idx].ctHash());
  } catch (const Index::NoSuchMeta&) {
    qCDebug(log_downloader) << "Expired Meta";
    // Well, remote node notifies us about expired meta. It was not requested by us OR another peer
    // sent us newer meta, so this had been expired. Nevertheless, ignore this notification.
  }
}
void Downloader::notifyRemoteChunk(Peer* peer, const QByteArray& ct_hash) {
  DownloadChunkPtr chunk = down_chunks_.value(ct_hash);
  if (!chunk) return;

  chunk->owned_by.insert(peer, peer->getInterestGuard());
  download_queue_.setPeerCount(ct_hash, chunk->owned_by.size());

  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::handleChoke(Peer* peer) {
  /* Remove requests to this node */
  for (DownloadChunkPtr missing_chunk : qAsConst(down_chunks_))
    missing_chunk->requests.remove(peer);

  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::handleUnchoke(Peer* peer) {
  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::putBlock(const QByteArray& ct_hash, uint32_t offset, const QByteArray& data, Peer* from) {
  auto missing_chunk = down_chunks_.value(ct_hash);
  if (!missing_chunk) return;

  QList<QPair<QByteArray, QFile*>> downloaded_chunks;

  QMutableHashIterator<Peer*, DownloadChunk::BlockRequest> request_it(missing_chunk->requests);
  while (request_it.hasNext()) {
    request_it.next();

    if (request_it.value().offset == offset              // Chunk position incorrect
        && request_it.value().size == (uint)data.size()  // Chunk size incorrect
        && request_it.key() ==
               from) {  // Requested node != replied. Well, it isn't critical, but will be useful to ban "fake" peers
      request_it.remove();

      missing_chunk->builder.put_block(offset, data);
      if (missing_chunk->builder.complete()) {
        QFile* chunk_f = missing_chunk->builder.release_chunk();
        chunk_f->setParent(this);

        downloaded_chunks << qMakePair(ct_hash, chunk_f);
      }  // TODO: catch "invalid hash" exception here
    }
  }

  for (QPair<QByteArray, QFile*> chunk : qAsConst(downloaded_chunks))
    emit chunkDownloaded(chunk.first, chunk.second);

  QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::trackPeer(Peer* peer) {
  peers_.insert(peer);
  download_queue_.setPeerCount(peers_.size());
}

void Downloader::untrackPeer(Peer* peer) {
  if (!peers_.contains(peer)) return;

  for (DownloadChunkPtr missing_chunk : down_chunks_.values()) {
    missing_chunk->requests.remove(peer);
    missing_chunk->owned_by.remove(peer);
    download_queue_.setPeerCount(missing_chunk->ct_hash, missing_chunk->owned_by.size());
  }
  peers_.remove(peer);
  download_queue_.setPeerCount(peers_.size());
}

void Downloader::maintainRequests() {
  // Prune old requests by timeout
  auto request_timeout = std::chrono::seconds(Config::get()->getGlobal("p2p_request_timeout").toUInt());
  for (DownloadChunkPtr missing_chunk : down_chunks_.values()) {
    QMutableHashIterator<Peer*, DownloadChunk::BlockRequest> request_it(missing_chunk->requests);
    while (request_it.hasNext())
      if (request_it.next().value().started + request_timeout < std::chrono::steady_clock::now())
        request_it.remove();
  }

  // Make new requests
  for (size_t i = countRequests(); i < Config::get()->getGlobal("p2p_download_slots").toUInt(); i++) {
    bool requested = requestOne();
    if (!requested) break;
  }
}

bool Downloader::requestOne() {
  // Try to choose chunk to request
  for (const QByteArray& ct_hash : download_queue_.chunks()) {
    // Try to choose a peer to request this block from
    auto peer = peerForRequest(ct_hash);
    if (!peer) continue;

    DownloadChunkPtr chunk = down_chunks_.value(ct_hash);

    // Rebuild request map to determine, which block to download now.
    AvailabilityMap<uint32_t> request_map = chunk->requestMap();

    // Request, actually
    if (!request_map.full()) {
      DownloadChunk::BlockRequest request;
      request.offset = request_map.begin()->first;
      request.size = std::min(
          request_map.begin()->second,
          uint32_t(Config::get()->getGlobal("p2p_block_size").toUInt())
      );
      request.started = std::chrono::steady_clock::now();

      protocol::v2::Message message;
      message.header.type = protocol::v2::MessageType::BLOCKREQUEST;
      message.blockrequest.ct_hash = ct_hash;
      message.blockrequest.offset = request.offset;
      message.blockrequest.length = request.size;
      peer->send(message);

      chunk->requests.insert(peer, request);
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

size_t Downloader::countRequests() const {
  size_t requests = 0;
  for (DownloadChunkPtr chunk : down_chunks_.values()) requests += chunk->requests.size();
  return requests;
}

} /* namespace librevault */
