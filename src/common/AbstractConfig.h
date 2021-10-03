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
#include <QJsonDocument>
#include <QObject>
#include <QVariant>

namespace librevault {

class AbstractConfig : public QObject {
  Q_OBJECT

 signals:
  void globalChanged(QString key, QVariant value);
  void folderAdded(QVariantMap fconfig);
  void folderRemoved(QByteArray folderid);

 public:
  /* Global configuration */
  virtual QVariant getGlobal(QString name) = 0;
  virtual void setGlobal(QString name, QVariant value) = 0;
  virtual void removeGlobal(QString name) = 0;

  /* Folder configuration */
  virtual void addFolder(QVariantMap fconfig) = 0;
  virtual void removeFolder(QByteArray folderid) = 0;

  virtual QVariantMap getFolder(QByteArray folderid) = 0;
  virtual QVariant getFolderValue(QByteArray folderid, const QString& name) { return getFolder(folderid).value(name); }
  virtual QList<QByteArray> listFolders() = 0;

  /* Export/Import */
  virtual QJsonDocument exportUserGlobals() = 0;
  virtual QJsonDocument exportGlobals() = 0;
  virtual void importGlobals(QJsonDocument globals_conf) = 0;

  virtual QJsonDocument exportUserFolders() = 0;
  virtual QJsonDocument exportFolders() = 0;
  virtual void importFolders(QJsonDocument folders_conf) = 0;
};

}  // namespace librevault
