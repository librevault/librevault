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
#include "Daemon.h"
#include "RemoteConfig.h"
#include "DaemonProcess.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QTimer>

Daemon::Daemon(QString control_url, QObject* parent) : QObject(parent) {
	daemon_url_ = control_url;
	process_ = new DaemonProcess(this);
	event_sock_ = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
	nam_ = new QNetworkAccessManager(this);
	remote_config_ = new RemoteConfig(this);

	// Connecting signals & slots
	connect(process_, &DaemonProcess::daemonReady, this, &Daemon::daemonUrlObtained);
	connect(process_, &DaemonProcess::daemonFailed, this, &Daemon::disconnected);

	connect(event_sock_, &QWebSocket::connected, this, &Daemon::connected);
	connect(event_sock_, &QWebSocket::disconnected, [this]{emit disconnected(event_sock_->closeReason().isEmpty() ? event_sock_->errorString() : event_sock_->closeReason());});
	connect(event_sock_, &QWebSocket::textMessageReceived, this, &Daemon::handleEventMessage);

	connect(this, &Daemon::connected, remote_config_, &RemoteConfig::renewConfig);
	connect(this, &Daemon::eventReceived, remote_config_, &RemoteConfig::handleEvent);

	connect(this, &Daemon::connected, []{qDebug() << "Daemon connection established";});
	connect(this, &Daemon::disconnected, [](QString message){qDebug() << "Daemon connection closed: " << message;});
}

Daemon::~Daemon() {}

bool Daemon::isConnected() {
	return event_sock_->isValid();
}

void Daemon::start() {
	if(daemon_url_.isEmpty())
		process_->launch();
	else
		emit daemonUrlObtained(QUrl(daemon_url_));
}

void Daemon::daemonUrlObtained(QUrl daemon_url) {
	daemon_url_ = daemon_url;
	qDebug() << "Daemon URL obtained: " << daemon_url_;

	QUrl event_url = daemon_url_;
	event_url.setScheme("ws");
	event_sock_->open(event_url);
}

void Daemon::handleEventMessage(const QString& message) {
	QJsonDocument event_msg_d = QJsonDocument::fromJson(message.toUtf8());
	QJsonObject event_msg_o = event_msg_d.object();

	QString event_type = event_msg_o["type"].toString();
	QJsonObject event_o = event_msg_o["event"].toObject();

	qDebug() << "EVENT id: " << event_msg_o["id"].toInt() << " t: " << event_type << " event: " << event_o;
	emit eventReceived(event_type, event_o);
}
