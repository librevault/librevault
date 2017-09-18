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
#include <control/FolderParams.h>
#include <PathNormalizer.h>
#include <SignedMeta.h>
#include "TaskScheduler.h"

namespace librevault {

TaskScheduler::TaskScheduler(const FolderParams& params, QObject* parent) : QObject(parent), params_(params) {

}

void TaskScheduler::scheduleScanning(const QString& abspath) {
  Q_ASSERT(params_.secret.canSign());

  QByteArray normpath = PathNormalizer::normalizePath(abspath, params_.path);
  QByteArray pt_keyed_hash = MetaInfo::makePathId(normpath, params_.secret);

  QMutexLocker lk(&tq_mtx);
  scan_tq.insert(pt_keyed_hash, abspath);

  process(pt_keyed_hash);
}

void TaskScheduler::scheduleAssemble(const SignedMeta& smeta) {
  Q_ASSERT(params_.secret.canDecrypt());

  QMutexLocker lk(&tq_mtx);
  assemble_tq.insert(smeta.metaInfo().pathKeyedHash(), smeta);

  process(smeta.metaInfo().pathKeyedHash());
}

void TaskScheduler::scheduleAddDownloadedMeta(const SignedMeta& smeta) {
  QMutexLocker lk(&tq_mtx);
  downloaded_tq.insert(smeta.metaInfo().pathKeyedHash(), smeta);

  process(smeta.metaInfo().pathKeyedHash());
}

Q_SLOT void TaskScheduler::process(const QByteArray& path_keyed_hash) {
  QMutexLocker lk(&tq_mtx);

  if(current_tasks.contains(path_keyed_hash) && !scan_tq.contains(path_keyed_hash))
    return;

}

} /* namespace librevault */
