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
#include <QJsonDocument>
#include "ControlClient.h"
#include "Daemon.h"

ControlClient::ControlClient(QString control_url) :	QObject(), socket_(std::make_unique<QWebSocket>()), control_url_(control_url) {
	connect(socket_.get(), &QWebSocket::textMessageReceived, this, &ControlClient::handle_message);

	connect(socket_.get(), &QWebSocket::connected, this, &ControlClient::handle_connect);
	connect(socket_.get(), &QWebSocket::disconnected, this, &ControlClient::handle_disconnect);
}

ControlClient::~ControlClient() {}

void ControlClient::start() {
	emit connecting();
	if(control_url_.isEmpty()) {
		daemon_ = std::make_unique<Daemon>();
		connect(daemon_.get(), &Daemon::daemonReady, this, &ControlClient::connectDaemon);
		connect(daemon_.get(), &Daemon::daemonFailed, this, &ControlClient::handle_daemonfail);
		daemon_->launch();
	}else{
		connectDaemon(QUrl(control_url_));
	}
}

void ControlClient::connectDaemon(const QUrl& daemon_address) {
	control_url_ = daemon_address;
	qDebug() << "Connecting to daemon: " << daemon_address;
	socket_->open(daemon_address);
}

void ControlClient::handle_message(const QString& message) {
	QJsonDocument json_document = QJsonDocument::fromJson(message.toUtf8());
	qDebug() << "<== " << json_document;
	emit ControlJsonReceived(json_document.object());
}

void ControlClient::handle_connect() {
	qDebug() << "Connected to daemon: " << control_url_;
	emit connected();
}

void ControlClient::handle_disconnect() {
	QString error_string = socket_->closeReason().isEmpty() ? socket_->errorString() : socket_->closeReason();

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
	socket_->sendTextMessage(json_document.toJson());
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