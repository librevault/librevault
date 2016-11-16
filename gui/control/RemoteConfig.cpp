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
#include "ControlClient.h"
#include <QNetworkReply>
#include <QJsonArray>
#include <QJsonDocument>

RemoteConfig::RemoteConfig(ControlClient* cc, QObject* parent) : QObject(parent) {
	cc_ = cc;
	connect(cc_, &ControlClient::connected, this, &RemoteConfig::renewConfig);
	connect(cc_, &ControlClient::eventReceived, this, &RemoteConfig::handleEvent);
}

QJsonValue RemoteConfig::getValue(QString key) {
	return cached_config_[key];
}

void RemoteConfig::setValue(QString key, QJsonValue value, bool force_send) {
	if((cached_config_[key] != value || force_send) && cc_->isConnected()) {
		QNetworkRequest request(cc_->daemonURL().toString().append("/v1/globals/%1").arg(key));
		QString body;
		switch(value.type()) {
			case QJsonValue::Bool: body = value.toBool() ? "true" : "false";
				break;
			case QJsonValue::String: body.append('"').append(value.toString()).append('"');
				break;
			case QJsonValue::Null: body = "null";
				break;
			case QJsonValue::Array: body = QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact);
				break;
			case QJsonValue::Double: body = QString::number(value.toDouble());
				break;
			default: body = "undefined";
				break;
		}
		cc_->networkAccessManager()->put(request, body.toUtf8());
	}
}

void RemoteConfig::renewConfig() {
	if(cc_->isConnected()) {
		QNetworkRequest request(cc_->daemonURL().toString().append("/v1/globals"));
		QNetworkReply* reply = cc_->networkAccessManager()->get(request);
		connect(reply, &QNetworkReply::finished, [this, reply] {
			if(reply->error() == QNetworkReply::NoError) {
				cached_config_ = QJsonDocument::fromJson(reply->readAll()).object();
			}
		});
	}
}

void RemoteConfig::handleEvent(QString name, QJsonObject event) {
	if(name == "GLOBAL_CHANGE") {
		cached_config_[event["key"].toString()] = event["value"];
	}
}
