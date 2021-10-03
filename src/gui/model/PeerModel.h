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
class FolderModel;

class PeerModel : public QAbstractListModel {
  Q_OBJECT

 public:
  PeerModel(QByteArray folderid, Daemon* daemon, FolderModel* parent);
  ~PeerModel();

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  int columnCount(const QModelIndex& parent = QModelIndex()) const;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

 public slots:
  void refresh();

 private:
  Daemon* daemon_;
  QByteArray folderid_;

  enum class Column {
    CLIENT_NAME,
    ENDPOINT,
    USER_AGENT,
    DOWN_SPEED,
    UP_SPEED,
    DOWN_BYTES,
    UP_BYTES,

    COLUMN_COUNT
  };
};
