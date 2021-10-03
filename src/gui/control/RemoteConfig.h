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
#include <QJsonValue>

#include "AbstractConfig.h"
#include "GenericRemoteDictionary.h"

class Daemon;

class RemoteConfig : public librevault::AbstractConfig {
  Q_OBJECT

 signals:
  void changed();

 public:
  explicit RemoteConfig(Daemon* daemon);
  virtual ~RemoteConfig() {}

 public slots:
  /* Global configuration */
  QVariant getGlobal(QString name) override;
  void setGlobal(QString name, QVariant value) override;
  void removeGlobal(QString name) override;

  /* Folder configuration */
  void addFolder(QVariantMap fconfig) override;
  void removeFolder(QByteArray folderid) override;

  QVariantMap getFolder(QByteArray folderid) override;
  QList<QByteArray> listFolders() override;

  /* Export/Import */
  QJsonDocument exportUserGlobals() override;
  QJsonDocument exportGlobals() override;
  void importGlobals(QJsonDocument globals_conf) override;

  QJsonDocument exportUserFolders() override;
  QJsonDocument exportFolders() override;
  void importFolders(QJsonDocument folders_conf) override;

 private:
  QVariantMap cached_globals_;
  QMap<QByteArray, QVariantMap> cached_folders_;

  Daemon* daemon_;

  void renew();
  void handleGlobals(QJsonDocument globals);
  void handleFolders(QJsonDocument folders);
  void handleEvent(QString name, QJsonObject event);
};
