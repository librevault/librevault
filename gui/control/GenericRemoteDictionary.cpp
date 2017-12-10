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
#include "GenericRemoteDictionary.h"
#include "Daemon.h"
#include "Secret.h"
#include <QNetworkReply>
#include <QJsonArray>
#include <QJsonDocument>

GenericRemoteDictionary::GenericRemoteDictionary(Daemon* daemon, QString globals_request, QString folders_request, QString global_event, QString folder_event) :
	QObject(daemon), daemon_(daemon), globals_request_(globals_request), folders_request_(folders_request), global_event_(global_event), folder_event_(folder_event) {
	connect(daemon_, &Daemon::connected, this, &GenericRemoteDictionary::renew);
	connect(daemon_, &Daemon::eventReceived, this, &GenericRemoteDictionary::handleEvent);
}

QVariant GenericRemoteDictionary::getGlobalValue(QString key) {
	return global_cache_[key].toVariant();
}

QJsonValue GenericRemoteDictionary::getFolderValue(QByteArray folderid, QString key) {
	return folder_cache_[folderid][key];
}

QList<QByteArray> GenericRemoteDictionary::folderList() {
	auto folderids = folder_cache_.keys();
	std::sort(folderids.begin(), folderids.end());
	return folderids;
}

QString GenericRemoteDictionary::convertOutValue(QJsonValue value) {
	switch(value.type()) {
		case QJsonValue::Bool:
			return value.toBool() ? "true" : "false";
		case QJsonValue::String:
			return QString('"').append(value.toString()).append('"');
		case QJsonValue::Null:
			return "null";
		case QJsonValue::Array:
			return QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact);
		case QJsonValue::Double:
			return QString::number(value.toDouble());
		default:
			return "undefined";
	}
}

void GenericRemoteDictionary::renew() {
	QUrl globals_request_url = daemon_->daemonUrl();
	globals_request_url.setPath(globals_request_);
	QNetworkRequest globals_request(globals_request_url);
	QNetworkReply* globals_reply = daemon_->nam()->get(globals_request);
	connect(globals_reply, &QNetworkReply::finished, [this, globals_reply, globals_request_url] {
		if(globals_reply->error() == QNetworkReply::NoError) {
			global_cache_ = QJsonDocument::fromJson(globals_reply->readAll()).object();
			qDebug() << "Fetched: " << globals_request_url;
			emit changed();
		}
	});

	QUrl folders_request_url = daemon_->daemonUrl();
	folders_request_url.setPath(folders_request_);
	QNetworkRequest folders_request(folders_request_url);
	QNetworkReply* folders_reply = daemon_->nam()->get(folders_request);
	connect(folders_reply, &QNetworkReply::finished, [this, folders_reply, folders_request_url] {
		if(folders_reply->error() == QNetworkReply::NoError) {
			auto folders_array = QJsonDocument::fromJson(folders_reply->readAll()).array();
			for(auto folder : folders_array) {
				QJsonObject folder_object = folder.toObject();

				librevault::Secret secret(folder_object["secret"].toString());
				QByteArray folderid = secret.getHash();

				folder_cache_[folderid] = folder_object;
			}
			qDebug() << "Fetched: " << folders_request_url;
			emit changed();
		}
	});
}

void GenericRemoteDictionary::handleEvent(QString name, QJsonObject event) {
	if(name == global_event_) {
		QString key = event["key"].toString();
		QJsonValue value = event["value"];
		if(global_cache_[key] != value) {
			global_cache_[key] = value;
			emit changed();
		}
	}else if(name == folder_event_) {
		QByteArray folderid = QByteArray::fromHex(event["folderid"].toString().toLatin1());
		QString key = event["key"].toString();
		QJsonValue value = event["value"];
		if(folder_cache_.contains(folderid)) {
			if(folder_cache_[folderid][key] != value) {
				folder_cache_[folderid][key] = value;
				emit changed();
			}
		}
	}else if(name == "EVENT_FOLDER_ADDED") {
		QByteArray folderid = QByteArray::fromHex(event["folderid"].toString().toLatin1());
		folder_cache_[folderid] = event["folder_params"].toObject();
		emit changed();
	}else if(name == "EVENT_FOLDER_REMOVED") {
		QByteArray folderid = QByteArray::fromHex(event["folderid"].toString().toLatin1());
		folder_cache_.remove(folderid);
		emit changed();
	}
}
