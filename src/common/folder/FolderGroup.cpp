#include "FolderGroup.h"

#include <QDir>
#include <QJsonArray>

#include "IgnoreList.h"
#include "PathNormalizer.h"
#include "control/StateCollector.h"
#include "folder/chunk/ChunkStorage.h"
#include "folder/meta/MetaStorage.h"
#include "folder/transfer/Downloader.h"
#include "folder/transfer/MetaDownloader.h"
#include "folder/transfer/MetaUploader.h"
#include "folder/transfer/Uploader.h"
#include "p2p/P2PFolder.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace librevault {

FolderGroup::FolderGroup(FolderParams params, StateCollector* state_collector, QObject* parent)
    : QObject(parent), params_(std::move(params)), state_collector_(state_collector) {
  LOGFUNC();

  /* Creating directories */
  QDir().mkpath(params_.path);
  QDir().mkpath(params_.system_path);
#ifdef Q_OS_WIN
  SetFileAttributesW(params_.system_path.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

  LOGD("New folder:"
       << "Key type=" << (char)params_.secret.get_type() << "Path=" << params_.path
       << "System path=" << params_.system_path);

  state_collector_->folder_state_set(params_.secret.get_Hash(), "secret", params_.secret.string());

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

  // Connecting signals and slots
  connect(meta_storage_, &MetaStorage::metaAdded, this, &FolderGroup::handleIndexedMeta);
  connect(chunk_storage_, &ChunkStorage::chunkAdded, this, [this](const QByteArray& ct_hash) {
    downloader_->notifyLocalChunk(ct_hash);
    uploader_->broadcast_chunk(remotes(), ct_hash);
  });
  connect(downloader_, &Downloader::chunkDownloaded, chunk_storage_, &ChunkStorage::put_chunk);
  connect(state_pusher_, &QTimer::timeout, this, &FolderGroup::push_state);

  // Set up state pusher
  state_pusher_->setInterval(1000);
  state_pusher_->start();

  // Go through index
  QTimer::singleShot(0, this, [=, this] {
    for (auto& smeta : meta_storage_->getMeta()) handleIndexedMeta(smeta);
  });
}

FolderGroup::~FolderGroup() {
  state_pusher_->stop();

  state_collector_->folder_state_purge(params_.secret.get_Hash());
  LOGFUNC();
}

/* Actions */
void FolderGroup::handleIndexedMeta(const SignedMeta& smeta) {
  auto revision = smeta.meta().path_revision();
  auto bitfield = chunk_storage_->make_bitfield(smeta.meta());

  downloader_->notifyLocalMeta(smeta, bitfield);
  meta_uploader_->broadcast_meta(remotes(), revision, bitfield);
}

// RemoteFolder actions
void FolderGroup::handle_handshake(RemoteFolder* origin) {
  remotes_ready_.insert(origin);
  downloader_->trackRemote(origin);

  connect(origin, &RemoteFolder::rcvdChoke, downloader_, [=, this] { downloader_->handleChoke(origin); });
  connect(origin, &RemoteFolder::rcvdUnchoke, downloader_, [=, this] { downloader_->handleUnchoke(origin); });
  connect(origin, &RemoteFolder::rcvdInterested, uploader_, [=, this] { uploader_->handle_interested(origin); });
  connect(origin, &RemoteFolder::rcvdNotInterested, uploader_, [=, this] { uploader_->handle_not_interested(origin); });

  connect(origin, &RemoteFolder::rcvdHaveMeta, meta_downloader_,
          [=, this](Meta::PathRevision revision, QBitArray bitfield) {
            meta_downloader_->handle_have_meta(origin, revision, bitfield);
          });
  connect(origin, &RemoteFolder::rcvdHaveChunk, downloader_,
          [=, this](const QByteArray& ct_hash) { downloader_->notifyRemoteChunk(origin, ct_hash); });
  connect(origin, &RemoteFolder::rcvdMetaRequest, meta_uploader_,
          [=, this](Meta::PathRevision path_revision) { meta_uploader_->handle_meta_request(origin, path_revision); });
  connect(origin, &RemoteFolder::rcvdMetaReply, meta_downloader_,
          [=, this](const SignedMeta& smeta, const QBitArray& bitfield) {
            meta_downloader_->handle_meta_reply(origin, smeta, bitfield);
          });
  connect(origin, &RemoteFolder::rcvdBlockRequest, uploader_,
          [=, this](const QByteArray& ct_hash, uint32_t offset, uint32_t size) {
            uploader_->handle_block_request(origin, ct_hash, offset, size);
          });
  connect(origin, &RemoteFolder::rcvdBlockReply, downloader_,
          [=, this](const QByteArray& ct_hash, uint32_t offset, const QByteArray& block) {
            downloader_->putBlock(ct_hash, offset, block, origin);
          });

  QTimer::singleShot(0, meta_uploader_, [=, this] { meta_uploader_->handle_handshake(origin); });
}

bool FolderGroup::attach(P2PFolder* remote) {
  if (remotes_.contains(remote) || p2p_folders_digests_.contains(remote->digest()) ||
      p2p_folders_endpoints_.contains(remote->endpoint()))
    return false;

  remotes_.insert(remote);
  p2p_folders_endpoints_.insert(remote->endpoint());
  p2p_folders_digests_.insert(remote->digest());

  LOGD("Attached remote " << remote->displayName());

  connect(remote, &RemoteFolder::handshakeSuccess, this, [=, this] { handle_handshake(remote); });

  emit attached(remote);

  return true;
}

void FolderGroup::detach(P2PFolder* remote) {
  if (!remotes_.contains(remote)) return;

  emit detached(remote);
  downloader_->untrackRemote(remote);

  p2p_folders_digests_.remove(remote->digest());
  p2p_folders_endpoints_.remove(remote->endpoint());

  remotes_.remove(remote);
  remotes_ready_.remove(remote);

  LOGD("Detached remote " << remote->displayName());
}

QList<RemoteFolder*> FolderGroup::remotes() const { return remotes_.values(); }

QString FolderGroup::log_tag() const { return !params_.path.isEmpty() ? params_.path : params_.system_path; }

void FolderGroup::push_state() {
  // peers
  QJsonArray peers_array;
  for (auto& p2p_folder : remotes_) peers_array += p2p_folder->collect_state();

  state_collector_->folder_state_set(folderid(), "peers", peers_array);
  // bandwidth
  state_collector_->folder_state_set(folderid(), "traffic_stats", bandwidth_counter_.heartbeat_json());
}

}  // namespace librevault
