#include "IndexerQueue.h"

#include <util/log.h>

#include "IndexerWorker.h"
#include "MetaStorage.h"
#include "control/FolderParams.h"
#include "control/StateCollector.h"
#include "folder/IgnoreList.h"
#include "folder/PathNormalizer.h"

Q_LOGGING_CATEGORY(log_indexer, "folder.meta.indexer")

namespace librevault {

IndexerQueue::IndexerQueue(const FolderParams& params, IgnoreList* ignore_list, PathNormalizer* path_normalizer,
                           StateCollector* state_collector, QObject* parent)
    : QObject(parent),
      params_(params),
      meta_storage_(qobject_cast<MetaStorage*>(parent)),
      ignore_list_(ignore_list),
      path_normalizer_(path_normalizer),
      state_collector_(state_collector),
      secret_(params.secret),
      scan_queue_({{}, {}, {}}) {
  qRegisterMetaType<SignedMeta>("SignedMeta");
  state_collector_->folder_state_set(secret_.get_Hash(), "is_indexing", false);

  connect(this, &IndexerQueue::startedIndexing, this,
          [this] { state_collector_->folder_state_set(secret_.get_Hash(), "is_indexing", true); });
  connect(this, &IndexerQueue::finishedIndexing, this,
          [this] { state_collector_->folder_state_set(secret_.get_Hash(), "is_indexing", false); });

  threadpool_ = new QThreadPool(this);

  scan_timer_ = new QTimer(this);
  scan_timer_->setTimerType(Qt::VeryCoarseTimer);
  scan_timer_->setInterval(1000);
  scan_timer_->start();
  connect(scan_timer_, &QTimer::timeout, this, &IndexerQueue::actualizeQueue);
}

IndexerQueue::~IndexerQueue() {
  SCOPELOG(log_indexer);
  emit aboutToStop();
  threadpool_->waitForDone();
}

void IndexerQueue::addIndexing(const QString& abspath) {
  for (auto& time_set : scan_queue_) time_set.remove(abspath);
  scan_queue_.last() += abspath;
}

void IndexerQueue::metaCreated(SignedMeta smeta) {
  IndexerWorker* worker = qobject_cast<IndexerWorker*>(sender());
  tasks_.remove(worker->absolutePath());
  worker->deleteLater();

  if (tasks_.size() == 0) emit finishedIndexing();

  meta_storage_->putMeta(smeta, true);
}

void IndexerQueue::metaFailed(QString error_string) {
  IndexerWorker* worker = qobject_cast<IndexerWorker*>(sender());
  tasks_.remove(worker->absolutePath());
  worker->deleteLater();

  if (tasks_.size() == 0) emit finishedIndexing();

  qCWarning(log_indexer) << "Skipping" << worker->absolutePath() << "Reason:" << error_string;
}

void IndexerQueue::actualizeQueue() {
  for (const auto& path : scan_queue_.takeFirst()) {
    qCDebug(log_indexer) << "Adding to indexerqueue:" << path;
    if (tasks_.contains(path)) {
      IndexerWorker* worker = tasks_.value(path);
      if (threadpool_->tryTake(worker))
        worker->deleteLater();
      else
        worker->stop();
    }

    auto* worker = new IndexerWorker(path, params_, meta_storage_, ignore_list_, path_normalizer_, this);
    worker->setAutoDelete(false);

    connect(this, &IndexerQueue::aboutToStop, worker, &IndexerWorker::stop, Qt::DirectConnection);
    connect(worker, &IndexerWorker::metaCreated, this, &IndexerQueue::metaCreated, Qt::QueuedConnection);
    connect(worker, &IndexerWorker::metaFailed, this, &IndexerQueue::metaFailed, Qt::QueuedConnection);

    tasks_.insert(path, worker);
    if (tasks_.size() == 1) emit startedIndexing();

    threadpool_->start(worker);
  }
  scan_queue_.append({});
}

}  // namespace librevault
