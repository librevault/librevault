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
#include <QObject>
#include <QVariant>

#include "AbstractConfig.h"

namespace librevault {

class Config : public AbstractConfig {
  Q_OBJECT
 public:
  ~Config();

  /* Exceptions */
  struct samekey_error : std::runtime_error {
    samekey_error()
        : std::runtime_error(
              "Multiple directories with the same key (or derived from the same key) are not supported") {}
  };

  static Config* get() {
    if (!instance_) instance_ = new Config();
    return instance_;
  }
  static void deinit() {
    delete instance_;
    instance_ = nullptr;
  }

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

 protected:
  Config();

  static Config* instance_;

 private:
  QJsonObject globals_custom_, globals_defaults_, folders_defaults_;
  QMap<QByteArray, QJsonObject> folders_custom_;

  void make_defaults();

  QJsonObject make_merged(QJsonObject custom_value, QJsonObject default_value) const;

  // File config
  void load();
  void save();
};

}  // namespace librevault
