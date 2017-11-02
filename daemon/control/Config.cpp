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
#include "Config.h"
#include "Paths.h"
#include "Secret.h"
#include "util/MergePatch.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QSaveFile>

Q_LOGGING_CATEGORY(log_config, "config")

namespace librevault {

Config::Config() {
  globals_defaults_ = readDefault(":/config/globals.json");
  globals_defaults_["client_name"] = QSysInfo::machineHostName();
  folders_defaults_ = readDefault(":/config/folders.json");

  load();

  connect(this, &Config::globalChanged, this, [this](QString key, QVariant value) {
    qCInfo(log_config) << "Global var" << key << "is set to" << value;
  });
  connect(this, &Config::globalChanged, this, &Config::save);
}

Config::~Config() { save(); }

Config* Config::instance_ = nullptr;

QVariant Config::getGlobal(QString name) {
  return mergePatch(globals_defaults_.value(name), globals_custom_.value(name)).toVariant();
}

void Config::setGlobal(QString name, QVariant value) {
  globals_custom_[name] = QJsonValue::fromVariant(value);
  emit globalChanged(name, value);
}

QJsonDocument Config::exportUserGlobals() { return QJsonDocument(globals_custom_); }

QJsonDocument Config::exportGlobals() {
  return QJsonDocument(mergePatch(globals_defaults_, globals_custom_).toObject());
}

void Config::importGlobals(QJsonDocument globals_conf) {
  QStringList all_keys = globals_custom_.keys() + globals_conf.object().keys();
  QSet<QString> changed_keys;

  for (const auto& key : qAsConst(all_keys))
    if (globals_custom_.value(key) != globals_conf.object().value(key)) changed_keys.insert(key);

  globals_custom_ = globals_conf.object();

  // Notify other components
  for (const auto& key : qAsConst(changed_keys)) emit globalChanged(key, getGlobal(key));
}

void Config::addFolder(QJsonObject fconfig) {
  QByteArray folderid = Secret(fconfig["secret"].toString()).folderid();
  if (folders_custom_.contains(folderid)) throw samekey_error();
  folders_custom_.insert(folderid, fconfig);
  emit folderAdded(mergePatch(folders_defaults_, fconfig).toObject());
  save();
}

void Config::removeFolder(QByteArray folderid) {
  emit folderRemoved(QByteArray((const char*)folderid.data(), folderid.size()));
  folders_custom_.remove(folderid);
  save();
}

QJsonObject Config::getFolder(QByteArray folderid) {
  return folders_custom_.contains(folderid)
             ? mergePatch(folders_defaults_, folders_custom_[folderid]).toObject()
             : QJsonObject();
}

QList<QByteArray> Config::listFolders() { return folders_custom_.keys(); }

QJsonDocument Config::exportUserFolders() {
  QJsonArray folders_merged;
  for (const auto& folder_params : folders_custom_.values()) folders_merged.append(folder_params);
  return QJsonDocument(folders_merged);
}

QJsonDocument Config::exportFolders() {
  QJsonArray folders_merged;
  for (const auto& folderid : listFolders()) folders_merged.append(getFolder(folderid));
  return QJsonDocument(folders_merged);
}

void Config::importFolders(QJsonDocument folders_conf) {
  for (const auto& folderid : folders_custom_.keys()) removeFolder(folderid);
  for (const auto& folder_params_v : folders_conf.array())
    addFolder(folder_params_v.toObject());
}

QJsonObject Config::readDefault(const QString& path) {
  QFile defaults_f(path);
  defaults_f.open(QIODevice::ReadOnly);

  QJsonDocument defaults = QJsonDocument::fromJson(defaults_f.readAll());
  Q_ASSERT(defaults.isObject());

  return defaults.object();
}

void Config::load() {
  QFile globals_f(Paths::get()->client_config_path);
  if (globals_f.open(QIODevice::ReadOnly)) {
    qCDebug(log_config) << "Loading global configuration from:" << globals_f.fileName();
    importGlobals(QJsonDocument::fromJson(globals_f.readAll()));
  } else
    qCWarning(log_config) << "Could not load global configuration from:" << globals_f.fileName();

  QFile folders_f(Paths::get()->folders_config_path);
  if (folders_f.open(QIODevice::ReadOnly)) {
    qCDebug(log_config) << "Loading folder configuration from:" << folders_f.fileName();
    importFolders(QJsonDocument::fromJson(folders_f.readAll()));
  } else
    qCWarning(log_config) << "Could not load folder configuration from:" << folders_f.fileName();
}

void Config::save() {
  {
    QSaveFile globals_f(Paths::get()->client_config_path);
    if (globals_f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      qCDebug(log_config) << "Saving global configuration to:" << globals_f.fileName();
      globals_f.write(exportUserGlobals().toJson(QJsonDocument::Indented));
    }
    if (globals_f.isOpen() && globals_f.commit())
      qCDebug(log_config) << "Saved global configuration to:" << globals_f.fileName();
    else
      qCWarning(log_config) << "Could not save global configuration to:" << globals_f.fileName();
  }

  {
    QSaveFile folders_f(Paths::get()->folders_config_path);
    if (folders_f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      qCDebug(log_config) << "Saving folder configuration to:" << folders_f.fileName();
      folders_f.write(exportUserFolders().toJson(QJsonDocument::Indented));
    }
    if (folders_f.isOpen() && folders_f.commit())
      qCDebug(log_config) << "Saved folder configuration to:" << folders_f.fileName();
    else
      qCWarning(log_config) << "Could not save folder configuration to:" << folders_f.fileName();
  }
}

}  // namespace librevault
