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
#include "FolderModel.h"

#include <QFileIconProvider>
#include <QSet>

#include "PeerModel.h"
#include "Secret.h"
#include "control/Daemon.h"
#include "control/RemoteConfig.h"
#include "control/RemoteState.h"
#include "util/human_size.h"

FolderModel::FolderModel(Daemon *daemon) : QAbstractListModel(daemon), daemon_(daemon) {}

FolderModel::~FolderModel() {}

int FolderModel::rowCount(const QModelIndex &parent) const { return folders_all_.size(); }
int FolderModel::columnCount(const QModelIndex &parent) const { return (int)Column::COLUMN_COUNT; }
QVariant FolderModel::data(const QModelIndex &index, int role) const {
  auto column = (Column)index.column();

  auto folderid = folders_all_[index.row()];

  if (role == Qt::DisplayRole) {
    switch (column) {
      case Column::NAME:
        return daemon_->config()->getFolderValue(folderid, "path").toString();
      case Column::STATUS:
        return [&, this] {
          if (daemon_->state()->getFolderValue(folderid, "is_indexing").toBool()) return tr("Indexing");
          return QString();
        }();
      case Column::PEERS:
        return tr("%n peer(s)", "", daemon_->state()->getFolderValue(folderid, "peers").toArray().size());
      case Column::SIZE: {
        QJsonObject index = daemon_->state()->getFolderValue(folderid, "index").toObject();
        return tr("%n file(s)", "", index["0"].toInt()) + " " + tr("%n directory(s)", "", index["1"].toInt());
      }

      default:
        return QVariant();
    }
  }
  if (role == Qt::DecorationRole && column == Column::NAME) return QFileIconProvider().icon(QFileIconProvider::Folder);
  if (role == HashRole) {
    return folderid;
  }

  return QVariant();
}

QVariant FolderModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch ((Column)section) {
        case Column::NAME:
          return tr("Name");
        case Column::STATUS:
          return tr("Status");
        case Column::PEERS:
          return tr("Peers");
        case Column::SIZE:
          return tr("Size");
        default:
          return QVariant();
      }
    }
  }
  return QVariant();
}

void FolderModel::refresh() {
  QSet<QByteArray> config_folders, new_folders, cleanup_folders;

  auto config_folders_list = daemon_->config()->listFolders();
  config_folders = QSet<QByteArray>(config_folders_list.begin(), config_folders_list.end());
  new_folders = config_folders - QSet<QByteArray>(folders_all_.begin(), folders_all_.end());
  cleanup_folders = QSet<QByteArray>(folders_all_.begin(), folders_all_.end()) - config_folders;

  for (auto folderid : new_folders) {
    PeerModel *new_peer_model = new PeerModel(folderid, daemon_, this);
    peer_models_.insert(folderid, new_peer_model);
  }

  for (auto folderid : cleanup_folders) {
    auto it = peer_models_.find(folderid);
    delete it.value();
    peer_models_.erase(it);
  }

  folders_all_ = config_folders.values();

  for (auto folderid : folders_all_) {
    auto it = peer_models_.find(folderid);
    it.value()->refresh();
  }

  emit dataChanged(createIndex(0, 0), createIndex(rowCount(), columnCount()));
  emit layoutChanged();
}
