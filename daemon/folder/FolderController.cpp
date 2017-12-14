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
#include "FolderController.h"

#include "FolderGroup.h"
#include "control/Config.h"
#include "control/Paths.h"
#include "p2p/PeerPool.h"
#include "p2p/PeerServer.h"
#include <QJsonArray>

namespace librevault {

Q_LOGGING_CATEGORY(log_controller, "folder.controller")

FolderController::FolderController(Config* config, QObject* parent)
    : QObject(parent), config_(config), storage_(":/config/folders.json") {}

void FolderController::loadAll() {
  try {
    importAll(storage_.readConfig(Paths::get()->folders_config_path).array());
    config_imported_ = true;
  } catch (const std::exception& e) {
    qCWarning(log_controller) << "Could not import configuration. E:" << e.what();
    config_imported_ = false;
  }
}

void FolderController::unloadAll() {
  if (config_imported_) {
    storage_.writeConfig(QJsonDocument(exportAll()), Paths::get()->folders_config_path);
    for (const auto& folderid : list()) unloadFolder(folderid);
    config_imported_ = false;
  }
}

void FolderController::loadFolder(const QJsonObject& folder_settings) {
  try {
    auto folderid = Secret(folder_settings["secret"].toString()).folderid();
    if (groups_.contains(folderid)) throw samekey_error();

    auto fgroup = new FolderGroup(folder_settings, defaults(), config_, this);
    Q_ASSERT(folderid == fgroup->params().folderid());

    groups_[fgroup->params().folderid()] = fgroup;

    auto peer_pool = new PeerPool(fgroup->params(), node_key_, bt_, dht_, mcast_, config_, this);
    fgroup->setPeerPool(peer_pool);
    peerserver_->addPeerPool(fgroup->params().folderid(), peer_pool);

    qCInfo(log_controller) << "Folder loaded:" << fgroup->params().path << "as"
                           << fgroup->params().folderid().toHex();
  } catch (const std::exception& e) {
    qCInfo(log_controller) << "Folder not loaded. E:" << e.what();
    throw;
  }
}

void FolderController::unloadFolder(const QByteArray& folderid) {
  auto fgroup = groups_[folderid];
  groups_.remove(folderid);

  fgroup->deleteLater();
  qCInfo(log_controller) << "Folder unloaded:" << folderid.toHex();
}

QList<QByteArray> FolderController::list() const { return groups_.keys(); }

void FolderController::importAll(const QJsonArray& folder_configs) {
  // TODO: implement incremental reload
  for (const auto& folderid : list()) unloadFolder(folderid);
  for (const auto& folder_config : folder_configs) loadFolder(folder_config.toObject());
}

QJsonArray FolderController::exportAll() const {
  QJsonArray result;
  for (FolderGroup* group : groups_.values()) result << group->exportConfig();
  return result;
}

}  // namespace librevault
