/* Copyright (C) 2015-2016 Alexander Shishenko <GamePad64@gmail.com>
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
#include <QJsonArray>

FolderModel::FolderModel() :
		QAbstractListModel() {}

FolderModel::~FolderModel() {}

int FolderModel::rowCount(const QModelIndex &parent) const {
	state_json_["folders"].toArray().size();
}
int FolderModel::columnCount(const QModelIndex &parent) const {
	return (int)Column::COLUMN_COUNT;
}
QVariant FolderModel::data(const QModelIndex &index, int role) const {
	auto column = (Column)index.column();

	if(role == Qt::DisplayRole) {
		auto folder_object = state_json_["folders"].toArray().at(index.row()).toObject();
		switch(column) {
			case Column::NAME: return folder_object["path"].toString();
			case Column::PEERS: return tr("%n peer(s)", "", folder_object["peers"].toArray().size());
			case Column::SECRET: return folder_object["secret"].toString();
			default: return QVariant();
		}
	}
	if(role == Qt::DecorationRole && column == Column::NAME) {
		QFileIconProvider file_icon_provider;
		return file_icon_provider.icon(QFileIconProvider::Folder);
	}
	return QVariant();
}

QVariant FolderModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if(orientation == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			switch((Column)section) {
				case Column::NAME: return tr("Name");
				case Column::PEERS: return tr("Peers");
				case Column::SECRET: return tr("Secret");
				default: return QVariant();
			}
		}
	}
	return QVariant();
}

void FolderModel::handleControlJson(QJsonObject state_json) {
	state_json_ = state_json["state"].toObject();
	emit dataChanged(createIndex(0,0), createIndex(0, 1));
}
