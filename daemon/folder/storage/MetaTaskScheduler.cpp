/* Copyright (C) 2017 Alexander Shishenko <alex@shishenko.com>
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
#include "MetaTaskScheduler.h"
#include <control/FolderParams.h>
#include <QDebug>
#include <QLoggingCategory>
#include <QTimer>

namespace librevault {

Q_LOGGING_CATEGORY(log_scheduler, "folder.storage.scheduler")

MetaTaskScheduler::MetaTaskScheduler(const FolderParams& params, QObject* parent)
    : QObject(parent), params_(params), tq_mtx_(QMutex::Recursive) {
  threadpool_ = new QThreadPool(this);
}

MetaTaskScheduler::~MetaTaskScheduler() {
  emit aboutToStop();
  threadpool_->waitForDone();
}

void MetaTaskScheduler::process(const QByteArray& path_keyed_hash) {
  QTimer::singleShot(0, this, [=]{
  QMutexLocker lk(&tq_mtx_);

  Q_ASSERT(!path_keyed_hash.isEmpty());
  Q_ASSERT(tq_.contains(path_keyed_hash));

  MetaTaskQueue& current_tq = tq_[path_keyed_hash];

  // Sort tasks by kind, and pick only last
  std::stable_sort(current_tq.pending_tasks.begin(), current_tq.pending_tasks.end(),
      [](QueuedTask* task1, QueuedTask* task2) { return task1->taskKind() < task2->taskKind(); });

  // If current task is empty, and there are pending tasks
  if (current_tq.current_task == nullptr && !current_tq.pending_tasks.isEmpty()) {
    // Run new task
    QueuedTask* task = current_tq.pending_tasks.dequeue();
    current_tq.current_task = task;

    threadpool_->start(task);
  }

  // If task queue for current path is empty, then remove the queue itself
  if (current_tq.pending_tasks.isEmpty() && current_tq.current_task == nullptr)
    tq_.remove(path_keyed_hash);
  });
}

void MetaTaskScheduler::handleFinished(const QByteArray& path_keyed_hash, QueuedTask* task) {
  QMutexLocker lk(&tq_mtx_);

  Q_ASSERT(tq_.contains(path_keyed_hash));
  Q_ASSERT(tq_[path_keyed_hash].current_task == task);

  tq_[path_keyed_hash].current_task = nullptr;
  qCDebug(log_scheduler) << "Finished task:" << task << "PKH:" << path_keyed_hash.toHex();

  process(path_keyed_hash);
}

void MetaTaskScheduler::scheduleTask(QueuedTask* task) {
  QMutexLocker lk(&tq_mtx_);

  auto path_keyed_hash = task->pathKeyedHash();
  Q_ASSERT(!path_keyed_hash.isEmpty());

  connect(task, &QObject::destroyed, this, [=] { handleFinished(path_keyed_hash, task); },
      Qt::DirectConnection);
  connect(
      this, &MetaTaskScheduler::aboutToStop, task, &QueuedTask::interrupt, Qt::DirectConnection);

  tq_[path_keyed_hash].pending_tasks.enqueue(task);
  qCDebug(log_scheduler) << "Scheduled task:" << task << "PKH:" << path_keyed_hash.toHex();

  process(path_keyed_hash);
}

}  // namespace librevault
