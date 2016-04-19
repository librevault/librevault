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
#include <QJsonDocument>
#include "ControlClient.h"

ControlClient::ControlClient() :
		QWebSocket() {
	connect(this, &QWebSocket::textMessageReceived, this, &ControlClient::handle_message);
	connect(this, &QWebSocket::connected, this, &ControlClient::handle_connect);
	connect(this, &QWebSocket::disconnected, this, &ControlClient::handle_disconnect);
}

ControlClient::~ControlClient() {}

void ControlClient::handle_message(const QString& message) {
	QJsonDocument json_document = QJsonDocument::fromJson(message.toUtf8());
	qDebug() << "<== " << json_document;
	emit ControlJsonReceived(json_document.object());
}

void ControlClient::handle_connect() {
	qDebug() << "Connected to daemon: " << daemon_address_;
}

void ControlClient::handle_disconnect() {
	qDebug() << "Disconnected from daemon: " << daemon_address_;
}

void ControlClient::connectDaemon(const QUrl& daemon_address) {
	daemon_address_ = daemon_address;
	qDebug() << "Connecting to daemon: " << daemon_address;
	open(daemon_address);
}

void ControlClient::sendControlJson(QJsonObject control_json) {
	QJsonDocument json_document(control_json);
	qDebug() << "==> " << json_document;
	this->sendTextMessage(json_document.toJson());
}

void ControlClient::sendConfigJson(QJsonObject config_json) {
	QJsonObject control_json;
	control_json["command"] = QStringLiteral("set_config");
	control_json["globals"] = config_json;
	sendControlJson(control_json);
}

void ControlClient::sendAddFolderJson(QString secret, QString path) {
	QJsonObject control_json, folder_json;
	folder_json["secret"] = secret;
	folder_json["path"] = path;

	control_json["command"] = QStringLiteral("add_folder");
	control_json["folder"] = folder_json;
	sendControlJson(control_json);
}

void ControlClient::sendRemoveFolderJson(QString secret) {
	QJsonObject control_json;
	control_json["command"] = QStringLiteral("remove_folder");
	control_json["secret"] = secret;
	sendControlJson(control_json);
}