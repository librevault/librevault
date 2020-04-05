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
#include "PeerModel.h"
#include "FolderModel.h"
#include "control/Daemon.h"
#include "control/RemoteState.h"
#include "util/human_size.h"
#include <QFileIconProvider>

PeerModel::PeerModel(QByteArray folderid, Daemon* daemon, FolderModel* parent) :
		QAbstractListModel(parent), daemon_(daemon), folderid_(folderid) {}

PeerModel::~PeerModel() {}

int PeerModel::rowCount(const QModelIndex &parent) const {
	return daemon_->state()->getFolderValue(folderid_, "peers").toArray().size();
}
int PeerModel::columnCount(const QModelIndex &parent) const {
	return (int)Column::COLUMN_COUNT;
}
QVariant PeerModel::data(const QModelIndex &index, int role) const {
	auto column = (Column)index.column();

	QJsonObject peer_object = daemon_->state()->getFolderValue(folderid_, "peers").toArray().at(index.row()).toObject();
	QJsonObject traffic_stats = peer_object["traffic_stats"].toObject();

	if(role == Qt::DisplayRole) {
		switch(column) {
			case Column::CLIENT_NAME: return peer_object["client_name"].toString();
			case Column::ENDPOINT: return peer_object["endpoint"].toString();
			case Column::USER_AGENT: return peer_object["user_agent"].toString();
			case Column::DOWN_SPEED: return human_bandwidth(traffic_stats["down_bandwidth"].toDouble());
			case Column::UP_SPEED: return human_bandwidth(traffic_stats["up_bandwidth"].toDouble());
			case Column::DOWN_BYTES: return human_size(traffic_stats["down_bytes"].toDouble());
			case Column::UP_BYTES: return human_size(traffic_stats["up_bytes"].toDouble());

			default: return QVariant();
		}
	}

	return QVariant();
}

QVariant PeerModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if(orientation == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			switch((Column)section) {
				case Column::CLIENT_NAME: return tr("Name");
				case Column::ENDPOINT: return tr("Address");
				case Column::USER_AGENT: return tr("Client");
				case Column::DOWN_SPEED: return tr("Down speed");
				case Column::UP_SPEED: return tr("Up speed");
				case Column::DOWN_BYTES: return tr("Downloaded");
				case Column::UP_BYTES: return tr("Uploaded");
				default: return QVariant();
			}
		}
	}
	return QVariant();
}

void PeerModel::refresh() {
	emit dataChanged(createIndex(0,0), createIndex(rowCount(), columnCount()));
	emit layoutChanged();
}
