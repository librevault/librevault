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
#include "FolderGroup.h"

#include "IgnoreList.h"
#include "PathNormalizer.h"
#include "PeerPool.h"
#include "control/StateCollector.h"
#include "folder/chunk/ChunkStorage.h"
#include "folder/meta/MetaStorage.h"
#include "folder/transfer/MetaDownloader.h"
#include "folder/transfer/MetaUploader.h"
#include "folder/transfer/Uploader.h"
#include "folder/transfer/Downloader.h"
#include "p2p/MessageHandler.h"
#include <QDir>
#include <QJsonArray>
#ifdef Q_OS_WIN
#   include <windows.h>
#endif

namespace librevault {

Q_LOGGING_CATEGORY(log_folder, "log.folder");

FolderGroup::FolderGroup(FolderParams params, PeerPool* pool, StateCollector* state_collector, QObject* parent) :
	QObject(parent),
	params_(std::move(params)), pool_(pool),
	state_collector_(state_collector) {

	/* Creating directories */
	QDir().mkpath(params_.path);
	QDir().mkpath(params_.system_path);
#ifdef Q_OS_WIN
	SetFileAttributesW(params_.system_path.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

	qCDebug(log_folder) << "New folder:"
		<< "Key type=" << (char)params_.secret.get_type()
		<< "Path=" << params_.path
		<< "System path=" << params_.system_path;

	state_collector_->folder_state_set(conv_bytearray(params_.secret.get_Hash()), "secret", QString::fromStdString(params_.secret.string()));

	/* Initializing components */
	path_normalizer_ = std::make_unique<PathNormalizer>(params_);
	ignore_list = std::make_unique<IgnoreList>(params_, *path_normalizer_);

	meta_storage_ = new MetaStorage(params_, ignore_list.get(), path_normalizer_.get(), state_collector_, this);
	chunk_storage_ = new ChunkStorage(params_, meta_storage_, path_normalizer_.get(), this);

	uploader_ = new Uploader(chunk_storage_, this);
	downloader_ = new Downloader(params_, meta_storage_, this);
	meta_uploader_ = new MetaUploader(meta_storage_, chunk_storage_, this);
	meta_downloader_ = new MetaDownloader(meta_storage_, downloader_, this);

	state_pusher_ = new QTimer(this);

	pool_->setParent(this);

	// Connecting signals and slots
	connect(meta_storage_, &MetaStorage::metaAdded, this, &FolderGroup::handle_indexed_meta);
	connect(chunk_storage_, &ChunkStorage::chunkAdded, this, [this](const blob& ct_hash){
		downloader_->notifyLocalChunk(ct_hash);
		uploader_->broadcast_chunk(pool_->validPeers(), ct_hash);
	});
	connect(downloader_, &Downloader::chunkDownloaded, chunk_storage_, &ChunkStorage::put_chunk);
	connect(state_pusher_, &QTimer::timeout, this, &FolderGroup::push_state);

	connect(pool_, &PeerPool::newValidPeer, this, &FolderGroup::handleNewPeer);

	// Set up state pusher
	state_pusher_->setInterval(1000);
	state_pusher_->start();

	// Go through index
	QTimer::singleShot(0, this, [=]{
		for(auto& smeta : meta_storage_->getMeta())
			handle_indexed_meta(smeta);
	});
}

FolderGroup::~FolderGroup() {
	state_pusher_->stop();

	state_collector_->folder_state_purge(conv_bytearray(params_.secret.get_Hash()));
}

/* Actions */
void FolderGroup::handle_indexed_meta(const SignedMeta& smeta) {
	Meta::PathRevision revision = smeta.meta().path_revision();
	bitfield_type bitfield = chunk_storage_->make_bitfield(smeta.meta());

	downloader_->notifyLocalMeta(smeta, bitfield);
	meta_uploader_->broadcast_meta(pool_->validPeers(), revision, bitfield);
}

// RemoteFolder actions
void FolderGroup::handleNewPeer(Peer* peer) {
	downloader_->trackRemote(peer);

	// Messages
	connect(peer->messageHandler(), &MessageHandler::rcvdChoke, downloader_, [=]{downloader_->handleChoke(peer);});
	connect(peer->messageHandler(), &MessageHandler::rcvdUnchoke, downloader_, [=]{downloader_->handleUnchoke(peer);});
	connect(peer->messageHandler(), &MessageHandler::rcvdInterested, downloader_, [=]{uploader_->handle_interested(peer);});
	connect(peer->messageHandler(), &MessageHandler::rcvdNotInterested, downloader_, [=]{uploader_->handle_not_interested(peer);});

	connect(peer->messageHandler(), &MessageHandler::rcvdHaveMeta, meta_downloader_, [=](Meta::PathRevision revision, bitfield_type bitfield){
		meta_downloader_->handle_have_meta(peer, revision, bitfield);
	});
	connect(peer->messageHandler(), &MessageHandler::rcvdHaveChunk, downloader_, [=](const blob& ct_hash){
		downloader_->notifyRemoteChunk(peer, ct_hash);
	});
	connect(peer->messageHandler(), &MessageHandler::rcvdMetaRequest, meta_uploader_, [=](Meta::PathRevision path_revision){
		meta_uploader_->handle_meta_request(peer, path_revision);
	});
	connect(peer->messageHandler(), &MessageHandler::rcvdMetaReply, meta_downloader_, [=](const SignedMeta& smeta, const bitfield_type& bitfield){
		meta_downloader_->handle_meta_reply(peer, smeta, bitfield);
	});
	connect(peer->messageHandler(), &MessageHandler::rcvdBlockRequest, uploader_, [=](const blob& ct_hash, uint32_t offset, uint32_t size){
		uploader_->handle_block_request(peer, ct_hash, offset, size);
	});
	connect(peer->messageHandler(), &MessageHandler::rcvdBlockReply, downloader_, [=](const blob& ct_hash, uint32_t offset, const blob& block){
		downloader_->putBlock(ct_hash, offset, block, peer);
	});

	// States
	connect(peer, &Peer::disconnected, downloader_, [=]{downloader_->untrackRemote(peer);});

	QTimer::singleShot(0, meta_uploader_, [=]{meta_uploader_->handle_handshake(peer);});
}

void FolderGroup::push_state() {
	// peers
	QJsonArray peers_array;
	for(auto& peer : pool_->validPeers()) {
		peers_array.append(peer->collectState());
	}
	state_collector_->folder_state_set(params().folderid(), "peers", peers_array);
}

} /* namespace librevault */
