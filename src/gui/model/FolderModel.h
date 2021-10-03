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
#pragma once
#include <QAbstractTableModel>
#include <QJsonObject>

class Daemon;
class PeerModel;

class FolderModel : public QAbstractListModel {
  Q_OBJECT

 public:
  FolderModel(Daemon* daemon);
  ~FolderModel();

  static const int HashRole = Qt::UserRole;

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  int columnCount(const QModelIndex& parent = QModelIndex()) const;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

  PeerModel* getPeerModel(const QByteArray& hash) { return peer_models_[hash]; }

 public slots:
  void refresh();

 private:
  Daemon* daemon_;

  QList<QByteArray> folders_all_;
  QMap<QByteArray, PeerModel*> peer_models_;

  enum class Column {
    NAME,
    STATUS,
    PEERS,
    SIZE,

    COLUMN_COUNT
  };
};
