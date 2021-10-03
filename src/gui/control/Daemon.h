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
 */
#pragma once
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtWebSockets/QWebSocket>
#include <memory>

class DaemonProcess;
class RemoteConfig;
class RemoteState;
class Daemon : public QObject {
  Q_OBJECT

 public:
  Daemon(QString control_url, QObject* parent);
  ~Daemon();

  QNetworkAccessManager* nam() { return nam_; }
  QUrl daemonUrl() { return daemon_url_; }

  RemoteConfig* config() { return remote_config_; }
  RemoteState* state() { return remote_state_; }
  bool isConnected();

 public slots:
  void start();

 private:
  DaemonProcess* process_;
  QWebSocket* event_sock_;
  QNetworkAccessManager* nam_;

  RemoteConfig* remote_config_;
  RemoteState* remote_state_;

  QUrl daemon_url_;

 signals:
  void connected();
  void disconnected(QString message);

  void eventReceived(QString name, QJsonObject event);

 private slots:
  void daemonUrlObtained(QUrl daemon_url);

  void handleEventMessage(const QString& message);
};
