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
#include "p2p/PeerPool.h"
#include "util/MergePatch.h"
#include <QDir>
#ifdef Q_OS_WIN
#  include <windows.h>
#endif

namespace librevault {

Q_LOGGING_CATEGORY(log_folder, "folder");

FolderGroup::FolderGroup(
    const QJsonObject& folder_config, const QJsonObject& defaults, Config* config, QObject* parent)
    : QObject(parent), config_(config), folder_config_(folder_config), params_(mergePatch(defaults, folder_config).toObject()) {
  createServiceDirectory();

  qCDebug(log_folder) << mergePatch(defaults, folder_config).toObject();
  qCDebug(log_folder) << params_.secret;
  qCDebug(log_folder) << "New folder;"
                      << "Level:" << (char)params_.secret.level() << "Path:" << params_.path
                      << "System path:" << params_.effectiveSystemPath();

  // Initializing local storage
  ignore_list_ = new IgnoreList(params_, this);

  task_scheduler_ = new MetaTaskScheduler(params_, this);
  index_ = new Index(params_, this);
  chunk_storage_ = new ChunkStorage(this, index_, this);
  poller_ = new DirectoryPoller(ignore_list_, index_, this);
  watcher_ = new DirectoryWatcher(params_, ignore_list_, this);

  if (params_.secret.canSign()) poller_->setEnabled(true);

  connect(poller_, &DirectoryPoller::newPath, this, &FolderGroup::addIndexing);
  connect(watcher_, &DirectoryWatcher::newPath, this, &FolderGroup::addIndexing);
  connect(index_, &Index::metaAddedExternal, this, &FolderGroup::addAssemble);

  // Initializing P2P transfers
  uploader_ = new Uploader(chunk_storage_, this);
  downloader_ = new Downloader(params_, index_, config_, this);
  meta_uploader_ = new MetaUploader(index_, chunk_storage_, this);
  meta_downloader_ = new MetaDownloader(params_, index_, downloader_, task_scheduler_, this);

  // Connecting signals and slots
  connect(index_, &Index::metaAdded, this, &FolderGroup::notifyLocalMeta);
  connect(chunk_storage_, &ChunkStorage::chunkAdded, this, &FolderGroup::handleNewChunk);
  connect(downloader_, &Downloader::chunkDownloaded, chunk_storage_, &ChunkStorage::putChunk);

  // Go through index
  QTimer::singleShot(0, this, [=] {
    for (auto& smeta : index_->getMeta()) notifyLocalMeta(smeta);
  });
}

void FolderGroup::addIndexing(const QString& abspath) {
  if (!params_.secret.canSign()) {
    qCDebug(log_folder) << "Secret level is insufficient for indexing";
    return;
  }

  QueuedTask* task = new ScanTask(abspath, params(), index_, ignore_list_, this);
  task_scheduler_->scheduleTask(task);
}

void FolderGroup::addAssemble(SignedMeta smeta) {
  if (!params_.secret.canDecrypt()) {
    qCDebug(log_folder) << "Secret level is insufficient for assembling";
    return;
  }

  QueuedTask* task = new AssembleTask(smeta, params(), index_, chunk_storage_, this);
  task_scheduler_->scheduleTask(task);
}

/* Actions */
void FolderGroup::notifyLocalMeta(const SignedMeta& smeta) {
  Q_ASSERT(pool_);

  MetaInfo::PathRevision revision = smeta.metaInfo().path_revision();
  QBitArray bitfield = chunk_storage_->makeBitfield(smeta.metaInfo());

  downloader_->notifyLocalMeta(smeta, bitfield);
  meta_uploader_->broadcastMeta(pool_->validPeers(), smeta);
}

void FolderGroup::handleNewChunk(const QByteArray& ct_hash) {
  Q_ASSERT(pool_);

  downloader_->notifyLocalChunk(ct_hash);
  meta_uploader_->broadcastChunk(pool_->validPeers(), ct_hash);
  for (const auto& meta : index_->containingChunk(ct_hash)) addAssemble(meta);
}

// RemoteFolder actions
void FolderGroup::handleNewPeer(Peer* peer) {
  // Messages
  connect(peer, &Peer::received, this, [=](const protocol::v2::Message& msg) {
    switch (msg.header.type) {
      case protocol::v2::MessageType::CHOKE: downloader_->handleChoke(peer); break;
      case protocol::v2::MessageType::UNCHOKE: downloader_->handleUnchoke(peer); break;
      case protocol::v2::MessageType::INTEREST: uploader_->handleInterested(peer); break;
      case protocol::v2::MessageType::UNINTEREST: uploader_->handleNotInterested(peer); break;
      case protocol::v2::MessageType::INDEXUPDATE:
        meta_downloader_->handleIndexUpdate(
            peer, msg.indexupdate.revision, msg.indexupdate.bitfield);
        break;
      case protocol::v2::MessageType::METAREQUEST:
        meta_uploader_->handleMetaRequest(peer, msg.metarequest.revision);
        break;
      case protocol::v2::MessageType::METARESPONSE:
        meta_downloader_->handleMetaReply(peer, msg.metaresponse.smeta, msg.metaresponse.bitfield);
        break;
      case protocol::v2::MessageType::BLOCKREQUEST:
        uploader_->handleBlockRequest(
            peer, msg.blockrequest.ct_hash, msg.blockrequest.offset, msg.blockrequest.length);
        break;
      case protocol::v2::MessageType::BLOCKRESPONSE:
        downloader_->putBlock(
            msg.blockresponse.ct_hash, msg.blockresponse.offset, msg.blockresponse.content, peer);
        break;
      default:;  // Nothing can go wrong here
    }
  });

  // States
  connect(peer, &Peer::disconnected, downloader_, [=] { downloader_->untrackPeer(peer); });
  connect(peer, &Peer::disconnected, uploader_, [=] { uploader_->untrackPeer(peer); });

  downloader_->trackPeer(peer);
  meta_uploader_->handleHandshake(peer);
}

void FolderGroup::setPeerPool(PeerPool* pool) {
  pool_ = pool;
  pool_->setParent(this);

  connect(pool_, &PeerPool::newValidPeer, this, &FolderGroup::handleNewPeer);
}

void FolderGroup::createServiceDirectory() {
  QDir().mkpath(params_.effectiveSystemPath());
#ifdef Q_OS_WIN
  SetFileAttributesW(params_.system_path.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
}

} /* namespace librevault */
