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
#include "folder/meta/MetaStorage.h"
#include "p2p/MessageHandler.h"
#include "util/readable.h"
#include <QLoggingCategory>
#include <boost/range/adaptor/map.hpp>

namespace librevault {

Q_LOGGING_CATEGORY(log_downloader, "folder.downloader")

DownloadChunk::DownloadChunk(const FolderParams& params, QByteArray ct_hash, quint32 size) : builder(params.system_path, ct_hash, size), ct_hash(ct_hash) {}

AvailabilityMap<uint32_t> DownloadChunk::requestMap() {
	AvailabilityMap<uint32_t> request_map = builder.file_map();
		foreach(auto& request, requests.values())
			request_map.insert({request.offset, request.size});
	return request_map;
}

Downloader::Downloader(const FolderParams& params, MetaStorage* meta_storage, QObject* parent) :
	QObject(parent),
	params_(params),
	meta_storage_(meta_storage) {
	LOGFUNC();
	maintain_timer_ = new QTimer(this);
	connect(maintain_timer_, &QTimer::timeout, this, &Downloader::maintainRequests);
	maintain_timer_->setInterval(Config::get()->getGlobal("p2p_request_timeout").toInt()*1000);
	maintain_timer_->setTimerType(Qt::VeryCoarseTimer);
	maintain_timer_->start();
}

Downloader::~Downloader() {}

void Downloader::notifyLocalMeta(const SignedMeta& smeta, const bitfield_type& bitfield) {
	SCOPELOG(log_downloader);

	Q_ASSERT(bitfield.size() == smeta.meta().chunks().size());

	QList<QByteArray> incomplete_chunks;
	incomplete_chunks.reserve(smeta.meta().chunks().size());

	bool have_complete = false;
	bool have_incomplete = false;

	for(size_t chunk_idx = 0; chunk_idx < smeta.meta().chunks().size(); chunk_idx++) {
		auto& meta_chunk = smeta.meta().chunks().at(chunk_idx);

		QByteArray ct_hash = conv_bytearray(meta_chunk.ct_hash);

		if(bitfield[chunk_idx]) {
			have_complete = true;   // We have chunk, remove from missing
			removeChunk(ct_hash); // Do not mark connected chunks as clustered, because they will be marked inside the loop below.
		}else{
			have_incomplete = true; // We haven't this chunk, we need to download it
			addChunk(ct_hash, meta_chunk.size);
			incomplete_chunks << ct_hash;
		}
	}

	if(have_complete && have_incomplete) {
		foreach(QByteArray ct_hash, getMetaCluster(incomplete_chunks)) {
			download_queue_.markClustered(ct_hash);
		}
	}
}

void Downloader::addChunk(QByteArray ct_hash, quint32 size) {
	qCDebug(log_downloader) << "Added" << ct_hash_readable(ct_hash) << "to download queue";

	uint32_t padded_size = size % 16 == 0 ? size : ((size / 16) + 1) * 16;

	DownloadChunkPtr chunk = std::make_shared<DownloadChunk>(params_, ct_hash, padded_size);
	down_chunks_.insert(ct_hash, chunk);

	download_queue_.addChunk(ct_hash);
}

void Downloader::removeChunk(QByteArray ct_hash) {
	if(down_chunks_.contains(ct_hash)) {
		download_queue_.removeChunk(ct_hash);
		down_chunks_.remove(ct_hash);

		qCDebug(log_downloader) << "Removed" << ct_hash_readable(ct_hash) << "from download queue";
	}
}

void Downloader::notifyLocalChunk(const blob& ct_hash) {
	SCOPELOG(log_downloader);

	removeChunk(conv_bytearray(ct_hash));

	// Mark all other chunks "clustered"
	foreach(QByteArray cluster_hash, getCluster(conv_bytearray(ct_hash))) {
		download_queue_.markClustered(cluster_hash);
	}
}

QSet<QByteArray> Downloader::getCluster(QByteArray ct_hash) {
	QSet<QByteArray> cluster;

	foreach(const SignedMeta& smeta, meta_storage_->containingChunk(conv_bytearray(ct_hash))) {
		for(auto& chunk : smeta.meta().chunks()) {
			cluster << conv_bytearray(chunk.ct_hash);
		}
	}

	return cluster;
}

QSet<QByteArray> Downloader::getMetaCluster(QList<QByteArray> ct_hashes) {
	QSet<QByteArray> cluster;

	foreach(QByteArray ct_hash, ct_hashes) {
		cluster += getCluster(ct_hash);
	}

	return cluster;
}

void Downloader::notifyRemoteMeta(P2PFolder* remote, const Meta::PathRevision& revision, bitfield_type bitfield) {
	SCOPELOG(log_downloader);
	try {
		auto chunks = meta_storage_->getMeta(revision).meta().chunks();
		bitfield.resize(chunks.size(), 0);  // Because, incoming bitfield size is packed into octets, so it's size != chunk list size;
		for(size_t chunk_idx = 0; chunk_idx < chunks.size(); chunk_idx++)
			if(bitfield[chunk_idx])
				notifyRemoteChunk(remote, chunks[chunk_idx].ct_hash);
	}catch(MetaStorage::no_such_meta){
		qCDebug(log_downloader) << "Expired Meta";
		// Well, remote node notifies us about expired meta. It was not requested by us OR another peer sent us newer meta, so this had been expired.
		// Nevertheless, ignore this notification.
	}
}
void Downloader::notifyRemoteChunk(P2PFolder* remote, const blob& ct_hash) {
	SCOPELOG(log_downloader);
	QByteArray ct_hash_q = conv_bytearray(ct_hash);

	DownloadChunkPtr chunk = down_chunks_.value(ct_hash_q);
	if(! chunk)
		return;

	chunk->owned_by.insert(remote, remote->get_interest_guard());
	download_queue_.setRemotesCount(ct_hash_q, chunk->owned_by.size());

	QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::handleChoke(P2PFolder* remote) {
	SCOPELOG(log_downloader);

	/* Remove requests to this node */
	foreach(DownloadChunkPtr missing_chunk, down_chunks_)
		missing_chunk->requests.remove(remote);

	QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::handleUnchoke(P2PFolder* remote) {
	SCOPELOG(log_downloader);
	QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::putBlock(const blob& ct_hash, uint32_t offset, const blob& data, P2PFolder* from) {
	SCOPELOG(log_downloader);
	auto missing_chunk = down_chunks_.value(conv_bytearray(ct_hash));
	if(! missing_chunk) return;

	QList<QPair<QByteArray, QFile*>> downloaded_chunks;

	QMutableHashIterator<P2PFolder*, DownloadChunk::BlockRequest> request_it(missing_chunk->requests);
	while(request_it.hasNext()) {
		request_it.next();

		if(request_it.value().offset == offset      // Chunk position incorrect
		&& request_it.value().size == data.size()   // Chunk size incorrect
		&& request_it.key() == from) {              // Requested node != replied. Well, it isn't critical, but will be useful to ban "fake" peers
			request_it.remove();

			missing_chunk->builder.put_block(offset, QByteArray::fromRawData((const char*)data.data(), data.size()));
			if(missing_chunk->builder.complete()) {
				QFile* chunk_f = missing_chunk->builder.release_chunk();
				chunk_f->setParent(this);

				downloaded_chunks << qMakePair(conv_bytearray(ct_hash), chunk_f);
			}   // TODO: catch "invalid hash" exception here
		}
	}

	for(QPair<QByteArray, QFile*> chunk : downloaded_chunks) {
		emit chunkDownloaded(chunk.first, chunk.second);
	}

	QTimer::singleShot(0, this, &Downloader::maintainRequests);
}

void Downloader::trackRemote(P2PFolder* remote) {
	remotes_.insert(remote);
	download_queue_.setRemotesCount(remotes_.size());
}

void Downloader::untrackRemote(P2PFolder* remote) {
	SCOPELOG(log_downloader);

	if(! remotes_.contains(remote)) return;

	foreach(DownloadChunkPtr missing_chunk, down_chunks_.values()) {
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
		foreach(DownloadChunkPtr missing_chunk, down_chunks_.values()) {
			QMutableHashIterator<P2PFolder*, DownloadChunk::BlockRequest> request_it(missing_chunk->requests);
			while(request_it.hasNext()) {
				if(request_it.next().value().started + request_timeout < std::chrono::steady_clock::now())
					request_it.remove();
			}
		}
	}

	// Make new requests
	{
		for(size_t i = countRequests(); i < Config::get()->getGlobal("p2p_download_slots").toUInt(); i++) {
			bool requested = requestOne();
			if(!requested) break;
		}
	}
}

bool Downloader::requestOne() {
	SCOPELOG(log_downloader);
	// Try to choose chunk to request
	foreach(QByteArray ct_hash, download_queue_.chunks()) {
		// Try to choose a remote to request this block from
		auto remote = nodeForRequest(ct_hash);
		if(! remote) continue;

		DownloadChunkPtr chunk = down_chunks_.value(ct_hash);

		// Rebuild request map to determine, which block to download now.
		AvailabilityMap<uint32_t> request_map = chunk->requestMap();

		// Request, actually
		if(!request_map.full()) {
			DownloadChunk::BlockRequest request;
			request.offset = request_map.begin()->first;
			request.size = std::min(request_map.begin()->second, uint32_t(Config::get()->getGlobal("p2p_block_size").toUInt()));
			request.started = std::chrono::steady_clock::now();

			remote->messageHandler()->sendBlockRequest(conv_bytearray(ct_hash), request.offset, request.size);
			chunk->requests.insert(remote, request);
			return true;
		}
	}
	return false;
}

P2PFolder* Downloader::nodeForRequest(QByteArray ct_hash) {
	DownloadChunkPtr chunk = down_chunks_.value(ct_hash);
	if(! chunk)
		return nullptr;

	for(P2PFolder* owner_remote : chunk->owned_by.keys())
		if(owner_remote->isValid() && !owner_remote->peer_choking()) return owner_remote; // TODO: implement more smart peer selection algorithm, based on peer weights.

	return nullptr;
}

size_t Downloader::countRequests() const {
	size_t requests = 0;
	foreach(DownloadChunkPtr chunk, down_chunks_.values())
		requests += chunk->requests.size();
	return requests;
}

} /* namespace librevault */
