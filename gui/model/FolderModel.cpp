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
#include <util/human_size.h>
#include <librevault/Secret.h>
#include "../gui/FolderProperties.h"

FolderModel::FolderModel(QWidget* parent) :
		QAbstractListModel(), parent_widget_(parent) {}

FolderModel::~FolderModel() {}

int FolderModel::rowCount(const QModelIndex &parent) const {
	return state_json_["folders"].toArray().size();
}
int FolderModel::columnCount(const QModelIndex &parent) const {
	return (int)Column::COLUMN_COUNT;
}
QVariant FolderModel::data(const QModelIndex &index, int role) const {
	auto column = (Column)index.column();

	auto folder_object = state_json_["folders"].toArray().at(index.row()).toObject();

	if(role == Qt::DisplayRole) {
		switch(column) {
			case Column::NAME: return folder_object["path"].toString();
			case Column::STATUS: return [&, this](){
					if(folder_object["is_indexing"].toBool())
						return tr("Indexing");
					return QString();
				}();
			case Column::PEERS: return tr("%n peer(s)", "", folder_object["peers"].toArray().size());
			case Column::SIZE: return tr("%n file(s)", "", folder_object["file_count"].toInt()) + " " + human_size(folder_object["byte_size"].toDouble());

			default: return QVariant();
		}
	}
	if(role == Qt::DecorationRole && column == Column::NAME)
		return QFileIconProvider().icon(QFileIconProvider::Folder);
	if(role == SecretRole)
		return folder_object["secret"].toString();
	if(role == HashRole) {
		auto hash_vec = librevault::Secret(folder_object["secret"].toString().toStdString()).get_Hash();
		return QByteArray((const char*)hash_vec.data(), hash_vec.size());
	}

	return QVariant();
}

QVariant FolderModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if(orientation == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			switch((Column)section) {
				case Column::NAME: return tr("Name");
				case Column::STATUS: return tr("Status");
				case Column::PEERS: return tr("Peers");
				case Column::SIZE: return tr("Size");
				default: return QVariant();
			}
		}
	}
	return QVariant();
}

void FolderModel::handleControlJson(QJsonObject control_json) {
	state_json_ = control_json["state"].toObject();

	// Determining what old Secrets are not present now
	QSet<QByteArray> existing_set = properties_dialogs_.keys().toSet();
	QSet<QByteArray> arrived_set;
	for(const QJsonValue& folder_json : state_json_["folders"].toArray()) {
		auto hash_vec = librevault::Secret(folder_json.toObject().value("secret").toString().toStdString()).get_Hash();
		arrived_set.insert(QByteArray((const char*)hash_vec.data(), hash_vec.size()));
	}
	QSet<QByteArray> cleanup_set = existing_set - arrived_set;

	// Removing deleted dialogs;
	for(auto& cleanup_secret : cleanup_set) {
		delete properties_dialogs_[cleanup_secret];
		properties_dialogs_.remove(cleanup_secret);
	}

	// Updating/creating current dialogs
	for(const QJsonValue& folder_state_json : state_json_["folders"].toArray()) {
		auto secret = librevault::Secret(folder_state_json.toObject().value("secret").toString().toStdString());
		QByteArray hash((const char*)secret.get_Hash().data(), secret.get_Hash().size());

		QJsonValue folder_config_json;

		for(const QJsonValue& folder_config_json_tmp : control_json["folders"].toArray()) {
			auto secret2 = librevault::Secret(folder_config_json_tmp.toObject().value("secret").toString().toStdString());
			QByteArray hash2((const char*)secret2.get_Hash().data(), secret2.get_Hash().size());

			if(hash == hash2)
				folder_config_json = folder_config_json_tmp; break;
		}


		if(! properties_dialogs_.contains(hash))
			properties_dialogs_[hash] = new FolderProperties(secret, parent_widget_);

		properties_dialogs_[hash]->update(control_json, folder_config_json.toObject(), folder_state_json.toObject());
	}

	emit dataChanged(createIndex(0,0), createIndex(rowCount(), columnCount()));
	emit layoutChanged();
}
