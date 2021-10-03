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
#include "FolderService.h"

#include "FolderGroup.h"
#include "control/Config.h"
#include "control/StateCollector.h"
#include "folder/meta/IndexerQueue.h"
#include "util/log.h"

Q_LOGGING_CATEGORY(log_folderservice, "FolderService")

namespace librevault {

FolderService::FolderService(StateCollector* state_collector, QObject* parent)
    : QObject(parent), state_collector_(state_collector) {
  SCOPELOG(log_folderservice);
}

FolderService::~FolderService() {
  SCOPELOG(log_folderservice);
  stop();
}

void FolderService::run() {
  for (const QByteArray& folderid : Config::get()->listFolders()) initFolder(Config::get()->getFolder(folderid));

  connect(Config::get(), &Config::folderAdded, this, &FolderService::initFolder);
  connect(Config::get(), &Config::folderRemoved, this, &FolderService::deinitFolder);
}

void FolderService::stop() {
  for (const QByteArray& hash : groups_.keys()) deinitFolder(hash);
}

void FolderService::initFolder(const FolderParams& params) {
  SCOPELOG(log_folderservice);
  auto fgroup = new FolderGroup(params, state_collector_, this);
  groups_[fgroup->folderid()] = fgroup;

  emit folderAdded(fgroup);
  LOGD("Folder initialized: " << fgroup->folderid().toHex());
}

void FolderService::deinitFolder(const QByteArray& folderid) {
  SCOPELOG(log_folderservice);
  FolderGroup* fgroup = getGroup(folderid);
  emit folderRemoved(fgroup);

  groups_.remove(folderid);

  fgroup->deleteLater();
  LOGD("Folder deinitialized: " << folderid.toHex());
}

FolderGroup* FolderService::getGroup(const QByteArray& folderid) { return groups_.value(folderid, nullptr); }

}  // namespace librevault
