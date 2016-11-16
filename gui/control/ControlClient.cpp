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
#include "ControlClient.h"
#include "Daemon.h"
#include <QJsonDocument>

ControlClient::ControlClient(QString control_url, QObject* parent) : QObject(parent), control_url_(control_url) {
	event_sock_ = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
	nam_ = new QNetworkAccessManager(this);

	connect(event_sock_, &QWebSocket::textMessageReceived, this, &ControlClient::handle_message);

	connect(event_sock_, &QWebSocket::connected, this, &ControlClient::handle_connect);
	connect(event_sock_, &QWebSocket::disconnected, this, &ControlClient::handle_disconnect);
}

ControlClient::~ControlClient() {}

bool ControlClient::isConnected() {
	return event_sock_->isValid();
}

void ControlClient::start() {
	emit connecting();
	if(control_url_.isEmpty()) {
		daemon_ = new Daemon(this);
		connect(daemon_, &Daemon::daemonReady, this, &ControlClient::connectDaemon);
		connect(daemon_, &Daemon::daemonFailed, this, &ControlClient::handle_daemonfail);
		daemon_->launch();
	}else{
		connectDaemon(QUrl(control_url_));
	}
}

void ControlClient::connectDaemon(QUrl daemon_address) {
	control_url_ = daemon_address;
	qDebug() << "Connecting to daemon: " << daemon_address;
	daemon_address.setScheme("ws");
	event_sock_->open(daemon_address);
}

void ControlClient::handle_message(const QString& message) {
	QJsonDocument event_msg_d = QJsonDocument::fromJson(message.toUtf8());
	QJsonObject event_msg_o = event_msg_d.object();

	QString event_type = event_msg_o["type"].toString();
	QJsonObject event_o = event_msg_o["event"].toObject();

	qDebug() << "EVENT id: " << event_msg_o["id"].toInt() << " t: " << event_type << " event: " << event_o;
	emit eventReceived(event_type, event_o);
}

void ControlClient::handle_connect() {
	qDebug() << "Connected to daemon: " << control_url_;
	emit connected();
}

void ControlClient::handle_disconnect() {
	QString error_string = event_sock_->closeReason().isEmpty() ? event_sock_->errorString() : event_sock_->closeReason();

	qDebug() << "Disconnected from daemon: " << control_url_ << " Reason: " << error_string;
	emit disconnected(error_string);
}

void ControlClient::handle_daemonfail(QString reason) {
	qDebug() << "Error running the service: " << reason;
	emit disconnected(reason);
}

void ControlClient::sendControlJson(QJsonObject control_json) {
	QJsonDocument json_document(control_json);
	qDebug() << "==> " << json_document;
	event_sock_->sendTextMessage(json_document.toJson());
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
