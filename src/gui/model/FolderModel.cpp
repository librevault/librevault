#include "FolderModel.h"

#include <QFileIconProvider>
#include <QSet>

#include "PeerModel.h"
#include "Secret.h"
#include "control/Daemon.h"
#include "control/RemoteConfig.h"
#include "control/RemoteState.h"
#include "util/human_size.h"

FolderModel::FolderModel(Daemon *daemon, QObject *parent) : QAbstractListModel(parent), daemon_(daemon) {}

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
        return {};
    }
  }
  if (role == Qt::DecorationRole && column == Column::NAME) return QFileIconProvider().icon(QFileIconProvider::Folder);
  if (role == HashRole) {
    return folderid;
  }

  return {};
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
          return {};
      }
    }
  }
  return {};
}

void FolderModel::refresh() {
  QSet<QByteArray> config_folders, new_folders, cleanup_folders, old_folders;

  auto config_folders_list = daemon_->config()->listFolders();

  old_folders = {folders_all_.begin(), folders_all_.end()};
  config_folders = {config_folders_list.begin(), config_folders_list.end()};

  new_folders = config_folders - old_folders;
  cleanup_folders = old_folders - config_folders;

  for (const auto &folderid : new_folders) {
    auto new_peer_model = new PeerModel(folderid, daemon_, this);
    peer_models_.insert(folderid, new_peer_model);
  }

  for (const auto &folderid : cleanup_folders) {
    auto it = peer_models_.find(folderid);
    it.value()->deleteLater();
    peer_models_.erase(it);
  }

  folders_all_ = config_folders.values();

  for (const auto &groupid : folders_all_) {
    auto it = peer_models_.find(groupid);
    it.value()->refresh();
  }

  emit dataChanged(createIndex(0, 0), createIndex(rowCount(), columnCount()));
  emit layoutChanged();
}
