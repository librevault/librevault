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
#pragma once
#include "SignedMeta.h"
#include "control/FolderSettings.h"
#include <QBitArray>
#include <QObject>

namespace librevault {

class Peer;
class Index;
class Downloader;
class MetaTaskScheduler;

class MetaDownloader : public QObject {
  Q_OBJECT

 public:
  MetaDownloader(const models::FolderSettings& params, Index* index, Downloader* downloader,
      MetaTaskScheduler* task_scheduler, QObject* parent);

  DECLARE_EXCEPTION(CantDownload, "Meta is forbidden for download");
  DECLARE_EXCEPTION_DETAIL(InvalidSignature, CantDownload, "Meta signature is invalid");
  DECLARE_EXCEPTION_DETAIL(OldMeta, CantDownload, "Remote node notified us about an older Meta than ours");

  /* Message handlers */
  void handleIndexUpdate(Peer* peer, const MetaInfo::PathRevision& revision, QBitArray bitfield);
  void handleMetaReply(Peer* peer, const SignedMeta& smeta, QBitArray bitfield);

 private:
  const models::FolderSettings& params_;
  Index* index_;
  Downloader* downloader_;
  MetaTaskScheduler* task_scheduler_;
};

} /* namespace librevault */
