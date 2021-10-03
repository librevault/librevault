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
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QObject>

class Daemon;

class GenericRemoteDictionary : public QObject {
  Q_OBJECT

 public:
  explicit GenericRemoteDictionary(Daemon* daemon, QString globals_request, QString folders_request,
                                   QString global_event, QString folder_event);
  ~GenericRemoteDictionary() {}

  QVariant getGlobalValue(QString key);
  QJsonValue getFolderValue(QByteArray folderid, QString key);
  QList<QByteArray> folderList();

 signals:
  void changed();

 protected:
  Daemon* daemon_;
  QJsonObject global_cache_;
  QMap<QByteArray, QJsonObject> folder_cache_;

  QString convertOutValue(QJsonValue value);

 private:
  QString globals_request_;
  QString folders_request_;

  QString global_event_;
  QString folder_event_;

 private slots:
  void renew();
  void handleEvent(QString name, QJsonObject event);
};
