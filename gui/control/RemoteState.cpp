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
#include "RemoteState.h"
#include "Daemon.h"
#include <librevault/Secret.h>
#include <QNetworkReply>
#include <QJsonArray>
#include <QJsonDocument>

RemoteState::RemoteState(Daemon* daemon) : QObject(daemon), daemon_(daemon) {}

QJsonValue RemoteState::getValue(QString key) {
	return cached_global_state_[key];
}

void RemoteState::renewState() {
	QNetworkRequest request_globals(daemon_->daemonUrl().toString().append("/v1/globals"));
	QNetworkReply* reply_globals = daemon_->nam()->get(request_globals);
	connect(reply_globals, &QNetworkReply::finished, [this, reply_globals] {
		if(reply_globals->error() == QNetworkReply::NoError) {
			cached_global_state_ = QJsonDocument::fromJson(reply_globals->readAll()).object();
			qDebug() << "Fetched global state from daemon";
		}
	});

	QNetworkRequest request_folders(daemon_->daemonUrl().toString().append("/v1/folders/state"));
	QNetworkReply* reply_folders = daemon_->nam()->get(request_folders);
	connect(reply_folders, &QNetworkReply::finished, [this, reply_folders] {
		if(reply_folders->error() == QNetworkReply::NoError) {
			QJsonArray folders_state = QJsonDocument::fromJson(reply_folders->readAll()).array();
			cached_folder_state_.clear();
			for(auto state_item : folders_state) {
				QJsonObject state_item_object = state_item.toObject();

				librevault::Secret secret(state_item_object["secret"].toString().toStdString());
				QByteArray folderid((const char*)secret.get_Hash().data(), secret.get_Hash().size());

				cached_folder_state_[folderid] = state_item_object;
			}
			qDebug() << "Fetched folders state from daemon";
		}
	});
}

void RemoteState::handleEvent(QString name, QJsonObject event) {
	if(name == "EVENT_GLOBAL_STATE_CHANGED") {
		QString key = event["key"].toString();
		QJsonValue value = event["value"];
		if(cached_global_state_[key] != value) {
			cached_global_state_[key] = value;
			emit globalStateChanged(key, value);
		}
	}
}
