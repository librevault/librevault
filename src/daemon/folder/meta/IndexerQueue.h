#pragma once
#include <QMap>
#include <QString>
#include <QThreadPool>
#include <QtCore/QLinkedList>
#include <QtCore/QTimer>

#include "SignedMeta.h"

namespace librevault {

class FolderParams;
class MetaStorage;
class IgnoreList;
class PathNormalizer;
class StateCollector;
class IndexerWorker;
class IndexerQueue : public QObject {
  Q_OBJECT
 signals:
  void aboutToStop();

  void startedIndexing();
  void finishedIndexing();

 public:
  IndexerQueue(const FolderParams& params, IgnoreList* ignore_list, PathNormalizer* path_normalizer,
               StateCollector* state_collector, QObject* parent);
  virtual ~IndexerQueue();

 public slots:
  void addIndexing(const QString& abspath);

 private:
  const FolderParams& params_;
  MetaStorage* meta_storage_;
  IgnoreList* ignore_list_;
  PathNormalizer* path_normalizer_;
  StateCollector* state_collector_;

  QThreadPool* threadpool_;
  QTimer* scan_timer_;

  Secret secret_;

  QMap<QString, IndexerWorker*> tasks_;

  QLinkedList<QSet<QString>> scan_queue_;

 private slots:
  void metaCreated(SignedMeta smeta);
  void metaFailed(QString error_string);

  void actualizeQueue();
};

}  // namespace librevault
