#pragma once
#include <QThreadPool>
#include <QTimer>

#include "SignedMeta.h"

namespace librevault {

class PathNormalizer;

class Archive;
class MetaStorage;
class FolderParams;
class ChunkStorage;
class Secret;

class AssemblerQueue : public QObject {
  Q_OBJECT
 signals:
  void aboutToStop();

  void startedAssemble();
  void finishedAssemble();

 public:
  AssemblerQueue(const FolderParams& params, MetaStorage* meta_storage, ChunkStorage* chunk_storage,
                 PathNormalizer* path_normalizer, Archive* archive, QObject* parent);
  virtual ~AssemblerQueue();

 public slots:
  void addAssemble(SignedMeta smeta);

 private:
  const FolderParams& params_;
  MetaStorage* meta_storage_;
  ChunkStorage* chunk_storage_;
  PathNormalizer* path_normalizer_;
  Archive* archive_;

  QThreadPool* threadpool_;

  void periodicAssembleOperation();
  QTimer* assemble_timer_;
};

}  // namespace librevault
