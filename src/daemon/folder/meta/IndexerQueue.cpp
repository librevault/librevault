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
 */
#include "IndexerQueue.h"

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
      secret_(params.secret) {
  qRegisterMetaType<SignedMeta>("SignedMeta");
  state_collector_->folder_state_set(secret_.get_Hash(), "is_indexing", false);

  connect(this, &IndexerQueue::startedIndexing, this,
          [this] { state_collector_->folder_state_set(secret_.get_Hash(), "is_indexing", true); });
  connect(this, &IndexerQueue::finishedIndexing, this,
          [this] { state_collector_->folder_state_set(secret_.get_Hash(), "is_indexing", false); });

  threadpool_ = new QThreadPool(this);
}

IndexerQueue::~IndexerQueue() {
  qCDebug(log_indexer) << "~IndexerQueue";
  emit aboutToStop();
  threadpool_->waitForDone();
  qCDebug(log_indexer) << "!~IndexerQueue";
}

void IndexerQueue::addIndexing(QString abspath) {
  if (tasks_.contains(abspath)) {
    IndexerWorker* worker = tasks_.value(abspath);
    if (threadpool_->tryTake(worker))
      worker->deleteLater();
    else
      worker->stop();
  }
  auto* worker = new IndexerWorker(abspath, params_, meta_storage_, ignore_list_, path_normalizer_, this);
  worker->setAutoDelete(false);
  connect(this, &IndexerQueue::aboutToStop, worker, &IndexerWorker::stop, Qt::DirectConnection);
  connect(worker, &IndexerWorker::metaCreated, this, &IndexerQueue::metaCreated);
  connect(worker, &IndexerWorker::metaFailed, this, &IndexerQueue::metaFailed);
  tasks_.insert(abspath, worker);
  if (tasks_.size() == 1) emit startedIndexing();
  threadpool_->start(worker);
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

}  // namespace librevault
