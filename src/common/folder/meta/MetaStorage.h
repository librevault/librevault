#pragma once
#include <QObject>

#include "SignedMeta.h"

namespace librevault {

class DirectoryPoller;
class DirectoryWatcher;
class FolderParams;
class IgnoreList;
class Index;
class IndexerQueue;
class PathNormalizer;
class StateCollector;

class MetaStorage : public QObject {
  Q_OBJECT
 signals:
  void metaAdded(SignedMeta meta);
  void metaAddedExternal(SignedMeta meta);

 public:
  struct MetaNotFound : public std::runtime_error {
    MetaNotFound() : std::runtime_error("Requested Meta not found") {}
  };

  MetaStorage(const FolderParams& params, IgnoreList* ignore_list, PathNormalizer* path_normalizer,
              StateCollector* state_collector, QObject* parent);
  ~MetaStorage() override;

  bool haveMeta(const Meta::PathRevision& path_revision) noexcept;
  SignedMeta getMeta(const Meta::PathRevision& path_revision);
  SignedMeta getMeta(const QByteArray& path_id);
  QList<SignedMeta> getMeta();
  QList<SignedMeta> getExistingMeta();
  QList<SignedMeta> getIncompleteMeta();
  void putMeta(const SignedMeta& signed_meta, bool fully_assembled = false);
  QList<SignedMeta> containingChunk(const QByteArray& ct_hash);
  QPair<quint32, QByteArray> getChunkSizeIv(const QByteArray& ct_hash);

  // Assembled index
  void markAssembled(const QByteArray& path_id);
  bool isChunkAssembled(const QByteArray& ct_hash);

  bool putAllowed(const Meta::PathRevision& path_revision) noexcept;

  void prepareAssemble(QByteArray normpath, Meta::Type type, bool with_removal = false);

 private:
  Index* index_;
  IndexerQueue* indexer_;
  DirectoryPoller* poller_;
  DirectoryWatcher* watcher_;
};

}  // namespace librevault
