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
#include "MetaDownloader.h"
#include "Downloader.h"
#include "folder/FolderGroup.h"
#include "folder/storage/AddDownloadedTask.h"
#include "folder/storage/Index.h"
#include "folder/storage/MetaTaskScheduler.h"
#include "p2p/Peer.h"

namespace librevault {

Q_LOGGING_CATEGORY(log_metadownloader, "folder.transfer.metadownloader")

MetaDownloader::MetaDownloader(const FolderSettings& params, Index* index, Downloader* downloader,
    MetaTaskScheduler* task_scheduler, QObject* parent)
    : QObject(parent),
      params_(params),
      index_(index),
      downloader_(downloader),
      task_scheduler_(task_scheduler) {}

void MetaDownloader::handleIndexUpdate(
    Peer* peer, const MetaInfo::PathRevision& revision, QBitArray bitfield) {
  if (index_->haveMeta(revision))
    downloader_->notifyRemoteMeta(peer, revision, bitfield);
  else if (index_->putAllowed(revision)) {
    protocol::v2::Message request;
    request.header.type = protocol::v2::MessageType::METAREQUEST;
    request.metarequest.revision = revision;
    peer->send(request);
  } else
    qCDebug(log_metadownloader) << "Remote node notified us about an expired Meta";
}

void MetaDownloader::handleMetaReply(Peer* peer, const SignedMeta& smeta, QBitArray bitfield) {
  try {
    if (!smeta.isValid(params_.secret)) throw InvalidSignature();
    if (!index_->putAllowed(smeta.metaInfo().path_revision())) throw OldMeta();

    auto id = smeta.metaInfo().path_revision();

    QueuedTask* task = new AddDownloadedTask(smeta, index_, this);
    connect(task, &QObject::destroyed, this,
        [=] { downloader_->notifyRemoteMeta(peer, id, bitfield); });
    task_scheduler_->scheduleTask(task);
  } catch (const CantDownload& e) {
    qCDebug(log_metadownloader) << e.what();
  }
}

}  // namespace librevault
