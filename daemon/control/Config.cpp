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
#include "util/MergePatch.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QLoggingCategory>
#include <QSaveFile>

Q_LOGGING_CATEGORY(log_config, "config")

namespace librevault {

Config::Config(QObject* parent) : PersistentConfiguration(":/config/globals.json", parent) {
  globals_defaults_["client_name"] = QSysInfo::machineHostName();
  folders_defaults_ = readDefault(":/config/folders.json");

  load();

  //  connect(this, &Config::globalChanged, this, [this](QString key, QVariant value) {
  //    qCInfo(log_config) << "Global var" << key << "is set to" << value;
  //  });
  connect(this, &Config::changed, this, &Config::save);
}

Config::~Config() { save(); }

void Config::patchGlobals(const QJsonObject& patch) {
  globals_custom_ = mergePatch(globals_custom_, patch).toObject();
  emit changed();
}

QJsonObject Config::getGlobals() {
  return mergePatch(globals_defaults_, globals_custom_).toObject();
}

QJsonObject Config::exportUserGlobals() { return globals_custom_; }

QJsonObject Config::exportGlobals() {
  return mergePatch(globals_defaults_, globals_custom_).toObject();
}

void Config::importGlobals(QJsonDocument globals_conf) {
  globals_custom_ = globals_conf.object();
  emit changed();
}

void Config::addFolder(QJsonObject fconfig) {
  QByteArray folderid = Secret(fconfig["secret"].toString()).folderid();
  if (folders_custom_.contains(folderid)) throw samekey_error();
  folders_custom_.insert(folderid, fconfig);
  save();
}

void Config::removeFolder(QByteArray folderid) {
  folders_custom_.remove(folderid);
  save();
}

QJsonObject Config::getFolder(QByteArray folderid) {
  return mergePatch(folders_defaults_, folders_custom_.value(folderid)).toObject();
}

QList<QByteArray> Config::listFolders() { return folders_custom_.keys(); }

QJsonArray Config::exportUserFolders() {
  QJsonArray folders_merged;
  for (const auto& folder_params : folders_custom_.values()) folders_merged.append(folder_params);
  return folders_merged;
}

QJsonArray Config::exportFolders() {
  QJsonArray folders_merged;
  for (const auto& folderid : listFolders()) folders_merged.append(getFolder(folderid));
  return folders_merged;
}

void Config::importFolders(QJsonDocument folders_conf) {
  for (const auto& folderid : folders_custom_.keys()) removeFolder(folderid);
  for (const auto& folder_params_v : folders_conf.array()) addFolder(folder_params_v.toObject());
}

QJsonObject Config::readDefault(const QString& path) {
  QFile defaults_f(path);
  defaults_f.open(QIODevice::ReadOnly);

  QJsonDocument defaults = QJsonDocument::fromJson(defaults_f.readAll());
  Q_ASSERT(defaults.isObject());

  return defaults.object();
}

QJsonDocument Config::readConfig(const QString& source) {
  QFile config_file(source);
  if (config_file.open(QIODevice::ReadOnly)) {
    qCDebug(log_config) << "Loading configuration from:" << config_file.fileName();
    return QJsonDocument::fromJson(config_file.readAll());
  } else
    qCWarning(log_config) << "Could not load configuration from:" << config_file.fileName();
  return {};
}

void Config::writeConfig(const QJsonDocument& doc, const QString& target) {
  QSaveFile config_file(target);
  if (config_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qCDebug(log_config) << "Saving configuration to:" << config_file.fileName();
    config_file.write(doc.toJson(QJsonDocument::Indented));
  }
  if (config_file.isOpen() && config_file.commit())
    qCDebug(log_config) << "Saved configuration to:" << config_file.fileName();
  else
    qCWarning(log_config) << "Could not save configuration to:" << config_file.fileName();
}

void Config::load() {
  importGlobals(readConfig(Paths::get()->client_config_path));
  importFolders(readConfig(Paths::get()->folders_config_path));
}

void Config::save() {
  writeConfig(QJsonDocument(exportUserGlobals()), Paths::get()->client_config_path);
  writeConfig(QJsonDocument(exportUserFolders()), Paths::get()->folders_config_path);
}

}  // namespace librevault
