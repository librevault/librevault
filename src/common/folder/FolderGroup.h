#pragma once
#include <util/Endpoint.h>

#include <QHostAddress>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <set>

#include "Secret.h"
#include "SignedMeta.h"
#include "control/FolderParams.h"
#include "p2p/BandwidthCounter.h"
#include "util/conv_bitfield.h"

namespace librevault {

class RemoteFolder;
class P2PFolder;

class PathNormalizer;
class IgnoreList;
class StateCollector;

class ChunkStorage;
class MetaStorage;

class MetaUploader;
class MetaDownloader;
class Uploader;
class Downloader;

class FolderGroup : public QObject {
  Q_OBJECT
  friend class ControlServer;

 signals:
  void attached(P2PFolder* remote_ptr);
  void detached(P2PFolder* remote_ptr);

 public:
  FolderGroup(FolderParams params, StateCollector* state_collector, QObject* parent);
  ~FolderGroup() override;

  /* Membership management */
  bool attach(P2PFolder* remote);
  void detach(P2PFolder* remote);

  /* Getters */
  QList<RemoteFolder*> remotes() const;

  inline const FolderParams& params() const { return params_; }

  inline const Secret& secret() const { return params().secret; }
  QByteArray folderid() const { return secret().get_Hash(); }

  BandwidthCounter& bandwidth_counter() { return bandwidth_counter_; }

  QString log_tag() const;

 private:
  const FolderParams params_;
  StateCollector* state_collector_;

  std::unique_ptr<PathNormalizer> path_normalizer_;
  std::unique_ptr<IgnoreList> ignore_list;

  ChunkStorage* chunk_storage_;
  MetaStorage* meta_storage_;

  Uploader* uploader_;
  Downloader* downloader_;
  MetaUploader* meta_uploader_;
  MetaDownloader* meta_downloader_;

  BandwidthCounter bandwidth_counter_;

  QTimer* state_pusher_;

  /* Members */
  QSet<RemoteFolder*> remotes_;
  QSet<RemoteFolder*> remotes_ready_;

  // Member lookup optimization
  QSet<QByteArray> p2p_folders_digests_;
  QSet<Endpoint> p2p_folders_endpoints_;

 private slots:
  void push_state();
  void handleIndexedMeta(const SignedMeta& smeta);
  void handle_handshake(RemoteFolder* origin);
};

}  // namespace librevault
