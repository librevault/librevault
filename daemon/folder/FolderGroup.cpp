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
#include "folder/storage/AddDownloadedTask.h"
#include "folder/storage/ChunkStorage.h"
#include "folder/storage/assembler/AssembleTask.h"
#include "folder/storage/scanner/DirectoryPoller.h"
#include "folder/storage/scanner/DirectoryWatcher.h"
#include "folder/storage/scanner/ScanTask.h"
#include "folder/transfer/Downloader.h"
#include "folder/transfer/MetaDownloader.h"
#include "folder/transfer/MetaUploader.h"
#include "folder/transfer/Uploader.h"
#include "p2p/MessageHandler.h"
#include "p2p/PeerPool.h"
#include <QDir>
#ifdef Q_OS_WIN
#  include <windows.h>
#endif

namespace librevault {

Q_LOGGING_CATEGORY(log_folder, "log.folder");

FolderGroup::FolderGroup(FolderParams params, NodeKey* node_key, QObject* parent)
    : QObject(parent), params_(std::move(params)) {
  createServiceDirectory();

  qCDebug(log_folder) << "New folder:"
                      << "Key type=" << params_.secret.level() << "Path=" << params_.path
                      << "System path=" << params_.system_path;

  // Initializing local storage
  ignore_list = std::make_unique<IgnoreList>(params_);

  task_scheduler_ = new MetaTaskScheduler(params, this);
  index_ = new Index(params, this);
  chunk_storage_ = new ChunkStorage(this, index_, this);
  poller_ = new DirectoryPoller(ignore_list.get(), index_, this);
  watcher_ = new DirectoryWatcher(params, ignore_list.get(), this);

  if (params_.secret.canSign()) poller_->setEnabled(true);

  connect(poller_, &DirectoryPoller::newPath, this, &FolderGroup::addIndexing);
  connect(watcher_, &DirectoryWatcher::newPath, this, &FolderGroup::addIndexing);
  connect(index_, &Index::metaAddedExternal, this, &FolderGroup::addAssemble);

  // Initializing P2P transfers
  uploader_ = new Uploader(chunk_storage_, this);
  downloader_ = new Downloader(params_, index_, this);
  meta_uploader_ = new MetaUploader(index_, chunk_storage_, this);
  meta_downloader_ = new MetaDownloader(params_, index_, downloader_, this);

  pool_ = new PeerPool(params, node_key, this);

  // Connecting signals and slots
  connect(index_, &Index::metaAdded, this, &FolderGroup::handleNewMeta);
  connect(chunk_storage_, &ChunkStorage::chunkAdded, this, [this](QByteArray ct_hash) {
    downloader_->notifyLocalChunk(ct_hash);
    uploader_->broadcast_chunk(pool_->validPeers(), ct_hash);
  });
  connect(downloader_, &Downloader::chunkDownloaded, chunk_storage_, &ChunkStorage::put_chunk);
  connect(meta_downloader_, &MetaDownloader::metaDownloaded, this, &FolderGroup::handleMetaDownloaded);

  connect(pool_, &PeerPool::newValidPeer, this, &FolderGroup::handleNewPeer);

  // Go through index
  QTimer::singleShot(0, this, [=] {
    for (auto& smeta : index_->getMeta()) handleNewMeta(smeta);
  });
}

FolderGroup::~FolderGroup() = default;

void FolderGroup::addIndexing(QString abspath) {
  QueuedTask* task = new ScanTask(abspath, params(), index_, ignore_list.get(), this);
  task_scheduler_->scheduleTask(task);
}

void FolderGroup::addAssemble(SignedMeta smeta) {
  QueuedTask* task = new AssembleTask(smeta, params(), index_, chunk_storage_, this);
  task_scheduler_->scheduleTask(task);
}

void FolderGroup::handleMetaDownloaded(SignedMeta smeta) {
  QueuedTask* task = new AddDownloadedTask(smeta, index_, this);
  task_scheduler_->scheduleTask(task);
}

/* Actions */
void FolderGroup::handleNewMeta(const SignedMeta& smeta) {
  MetaInfo::PathRevision revision = smeta.metaInfo().path_revision();
  QBitArray bitfield = chunk_storage_->make_bitfield(smeta.metaInfo());

  downloader_->notifyLocalMeta(smeta, bitfield);
  meta_uploader_->broadcastMeta(pool_->validPeers(), revision, bitfield);
}

// RemoteFolder actions
void FolderGroup::handleNewPeer(Peer* peer) {
  downloader_->trackRemote(peer);

  // Messages
  connect(peer->messageHandler(), &MessageHandler::rcvdChoke, downloader_, [=] { downloader_->handleChoke(peer); });
  connect(peer->messageHandler(), &MessageHandler::rcvdUnchoke, downloader_, [=] { downloader_->handleUnchoke(peer); });
  connect(peer->messageHandler(), &MessageHandler::rcvdInterested, downloader_,
          [=] { uploader_->handle_interested(peer); });
  connect(peer->messageHandler(), &MessageHandler::rcvdNotInterested, downloader_,
          [=] { uploader_->handle_not_interested(peer); });

  connect(peer->messageHandler(), &MessageHandler::rcvdHaveMeta, meta_downloader_,
          [=](MetaInfo::PathRevision revision, QBitArray bitfield) {
            meta_downloader_->handleHaveMeta(peer, revision, bitfield);
          });
  connect(peer->messageHandler(), &MessageHandler::rcvdHaveChunk, downloader_,
          [=](QByteArray ct_hash) { downloader_->notifyRemoteChunk(peer, ct_hash); });
  connect(peer->messageHandler(), &MessageHandler::rcvdMetaRequest, meta_uploader_,
          [=](MetaInfo::PathRevision path_revision) { meta_uploader_->handleMetaRequest(peer, path_revision); });
  connect(
      peer->messageHandler(), &MessageHandler::rcvdMetaReply, meta_downloader_,
      [=](const SignedMeta& smeta, QBitArray bitfield) { meta_downloader_->handleMetaReply(peer, smeta, bitfield); });
  connect(peer->messageHandler(), &MessageHandler::rcvdBlockRequest, uploader_,
          [=](QByteArray ct_hash, uint32_t offset, uint32_t size) {
            uploader_->handle_block_request(peer, ct_hash, offset, size);
          });
  connect(peer->messageHandler(), &MessageHandler::rcvdBlockReply, downloader_,
          [=](QByteArray ct_hash, uint32_t offset, QByteArray block) {
            downloader_->putBlock(ct_hash, offset, block, peer);
          });

  // States
  connect(peer, &Peer::disconnected, downloader_, [=] { downloader_->untrackRemote(peer); });

  QTimer::singleShot(0, meta_uploader_, [=] { meta_uploader_->handleHandshake(peer); });
}

void FolderGroup::createServiceDirectory() {
  QDir().mkpath(params_.system_path);
#ifdef Q_OS_WIN
  SetFileAttributesW(params_.system_path.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
}

} /* namespace librevault */
