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
#include "RemoteConfig.h"
#include "Daemon.h"
#include "secret/Secret.h"
#include <QEventLoop>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>

RemoteConfig::RemoteConfig(Daemon* daemon) : daemon_(daemon) {
	connect(daemon_, &Daemon::connected, this, &RemoteConfig::renew);
	connect(daemon_, &Daemon::eventReceived, this, &RemoteConfig::handleEvent);

	connect(this, &librevault::AbstractConfig::changed, this, &RemoteConfig::changed);
	connect(this, &librevault::AbstractConfig::folderAdded, this, &RemoteConfig::changed);
	connect(this, &librevault::AbstractConfig::folderRemoved, this, &RemoteConfig::changed);
}

void RemoteConfig::setGlobal(QString name, QVariant value) {
	if(daemon_->isConnected()) {	// TODO: do not save if value is same
		QJsonObject value_setter;
		value_setter["key"] = name;
		value_setter["value"] = QJsonValue::fromVariant(value);

		QNetworkRequest request(daemon_->daemonUrl().toString().append("/v1/globals/%1").arg(name));
		daemon_->nam()->put(request, QJsonDocument(value_setter).toJson(QJsonDocument::Compact));
	}
}

void RemoteConfig::addFolder(QJsonObject fconfig) {
	if(daemon_->isConnected()) {
		librevault::Secret secret(fconfig["secret"].toString());
		QByteArray folderid = secret.folderid();

		QNetworkRequest request(daemon_->daemonUrl().toString().append("/v1/folders/%1").arg(QString(folderid.toHex())));
		daemon_->nam()->put(request, QJsonDocument(fconfig).toJson());
	}
}

void RemoteConfig::removeFolder(QByteArray folderid) {
	if(daemon_->isConnected()) {
		QNetworkRequest request(daemon_->daemonUrl().toString().append("/v1/folders/%1").arg(QString(folderid.toHex())));
		QNetworkReply* reply = daemon_->nam()->deleteResource(request);

		QEventLoop sync_loop;
		connect(reply, &QNetworkReply::finished, &sync_loop, &QEventLoop::quit);
		sync_loop.exec();

		if(reply->error()) {
			// handle somehow!
		}
	}
}

librevault::models::ClientSettings RemoteConfig::getGlobals() {
	return librevault::models::ClientSettings::fromJson(cached_globals_);
}

QJsonObject RemoteConfig::getFolder(QByteArray folderid) {
	return QJsonObject::fromVariantMap(cached_folders_.value(folderid));
}

QList<QByteArray> RemoteConfig::listFolders() {
	return cached_folders_.keys();
}

QJsonDocument RemoteConfig::exportGlobals() {
	return QJsonDocument::fromVariant(cached_globals_);
}

QJsonDocument RemoteConfig::exportFolders() {
	QJsonArray folders;
	foreach(const QVariantMap& folder, cached_folders_.values()) {
		folders.append(QJsonValue::fromVariant(folder));
	}
	return QJsonDocument(folders);
}

void RemoteConfig::renew() {
	{
		QUrl request_url = daemon_->daemonUrl();
		request_url.setPath("/v1/globals");
		QNetworkRequest request(request_url);
		QNetworkReply* reply = daemon_->nam()->get(request);
		connect(reply, &QNetworkReply::finished, [this, reply, request_url] {
			if(reply->error() == QNetworkReply::NoError) {
				handleGlobals(QJsonDocument::fromJson(reply->readAll()));
				qDebug() << "Fetched:" << request_url;
			}
		});
	}

	{
		QUrl request_url = daemon_->daemonUrl();
		request_url.setPath("/v1/folders");
		QNetworkRequest request(request_url);
		QNetworkReply* reply = daemon_->nam()->get(request);
		connect(reply, &QNetworkReply::finished, [this, reply, request_url] {
			if(reply->error() == QNetworkReply::NoError) {
				handleFolders(QJsonDocument::fromJson(reply->readAll()));
				qDebug() << "Fetched:" << request_url;
			}
		});
	}
}

void RemoteConfig::handleGlobals(QJsonDocument globals) {
	QJsonObject globals_new = globals.object();

	// Prepare notifications
	QStringList all_keys = globals_new.keys() + cached_globals_.keys();
	all_keys.removeDuplicates();

	QMutableListIterator<QString> it(all_keys);
	while(it.hasNext()) {
		QString key = it.next();
		if(globals_new.value(key) == cached_globals_.value(key))
			it.remove();
	}

	// Set
	cached_globals_ = globals_new;

	// Notify
	emit changed();
}

void RemoteConfig::handleFolders(QJsonDocument folders) {
	foreach(const QJsonValue& value, folders.array()) {
		QJsonObject folder_object = value.toObject();
		librevault::Secret secret(folder_object["secret"].toString());
		cached_folders_[secret.folderid()] = folder_object.toVariantMap();
	}
}

void RemoteConfig::handleEvent(QString name, QJsonObject event) {
	if(name == "EVENT_GLOBAL_CONFIG_CHANGED") {
		QString key = event["key"].toString();
		QJsonValue value = event["value"];
		if(cached_globals_.value(key) != value) {
			cached_globals_[key] = value;
			emit changed();
		}
	}else if(name == "EVENT_FOLDER_ADDED") {
		QByteArray folderid = QByteArray::fromHex(event["folderid"].toString().toLatin1());
		cached_folders_[folderid] = event["folder_params"].toObject().toVariantMap();
		emit folderAdded(QJsonObject::fromVariantMap(cached_folders_[folderid]));
	}else if(name == "EVENT_FOLDER_REMOVED") {
		QByteArray folderid = QByteArray::fromHex(event["folderid"].toString().toLatin1());
		cached_folders_.remove(folderid);
		emit folderRemoved(folderid);
	}
}
