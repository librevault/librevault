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
    : QObject(parent), params_(params) {
  threadpool_ = new QThreadPool(this);
}

MetaTaskScheduler::~MetaTaskScheduler() {
  emit aboutToStop();
  threadpool_->waitForDone();
}

void MetaTaskScheduler::process(QByteArray path_keyed_hash) {
  Q_ASSERT(!path_keyed_hash.isEmpty());

  QTimer::singleShot(0, this, [=] {
    QMutexLocker lk(&tq_mtx);

    // If no task queue is present, then no-op
    if (!pending_tasks.contains(path_keyed_hash)) return;

    // Sort tasks by kind, and pick only last
    QMap<QueuedTask::TaskKind, QueuedTask*> uniq_tasks;
    for (const auto& task : pending_tasks[path_keyed_hash]) {
      if (uniq_tasks.contains(task->taskKind())) uniq_tasks[task->taskKind()]->deleteLater();
      uniq_tasks[task->taskKind()] = task;
    }
    pending_tasks[path_keyed_hash] = uniq_tasks.values();

    // If current task is empty, and there are pending tasks
    if (!current_tasks.contains(path_keyed_hash) && pending_tasks.contains(path_keyed_hash)) {
      // Run new task
      QueuedTask* task = pending_tasks[path_keyed_hash].takeFirst();
      current_tasks[path_keyed_hash] = task;

      threadpool_->start(task);
    }

    // If task queue for current path is empty, then remove the queue itself
    if (pending_tasks[path_keyed_hash].isEmpty()) pending_tasks.remove(path_keyed_hash);
  });
}

void MetaTaskScheduler::handleFinished(QueuedTask* task) {
  qCDebug(log_scheduler) << "Finished task:" << task;

  QMutexLocker lk(&tq_mtx);

  QByteArray path_keyed_hash = current_tasks.key(task);
  current_tasks.remove(path_keyed_hash);
  process(path_keyed_hash);
}

void MetaTaskScheduler::scheduleTask(QueuedTask* task) {
  connect(task, &QObject::destroyed, this, [=] { handleFinished(task); }, Qt::DirectConnection);
  connect(
      this, &MetaTaskScheduler::aboutToStop, task, &QueuedTask::interrupt, Qt::DirectConnection);

  QMutexLocker lk(&tq_mtx);
  pending_tasks[task->pathKeyedHash()].append(task);
  qCDebug(log_scheduler) << "Scheduled task:" << task;

  process(task->pathKeyedHash());
}

}  // namespace librevault
