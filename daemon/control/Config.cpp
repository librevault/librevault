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

Config::Config(QObject* parent) : QObject(parent), storage_(":/config/globals.json") {
  //globals_defaults_["client_name"] = QSysInfo::machineHostName();

  load();

  //  connect(this, &Config::globalChanged, this, [this](QString key, QVariant value) {
  //    qCInfo(log_config) << "Global var" << key << "is set to" << value;
  //  });
  connect(this, &Config::changed, this, &Config::save);
}

Config::~Config() { save(); }

void Config::patch(const QJsonObject& patch) {
  settings_ = mergePatch(settings_, patch).toObject();
  emit changed();
}

QJsonValue Config::get(const QString& key) {
  return mergePatch(defaults().value(key), settings_.value(key));
}

QJsonObject Config::exportSettings() { return settings_; }

void Config::importSettings(QJsonDocument globals_conf) {
  settings_ = globals_conf.object();
  emit changed();
}

void Config::load() {
  importSettings(storage_.readConfig(Paths::get()->client_config_path));
}

void Config::save() {
  storage_.writeConfig(QJsonDocument(exportSettings()), Paths::get()->client_config_path);
}

}  // namespace librevault
