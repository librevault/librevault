/* Copyright (C) 2017 Alexander Shishenko <alex@shishenko.com>
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
#include "SettingsStorage.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QLoggingCategory>
#include <QSaveFile>

Q_LOGGING_CATEGORY(log_persistent, "config.persistent")

namespace librevault {

SettingsStorage::SettingsStorage(const QString& defaults_path)
    : defaults_(readDefault(defaults_path)) {}

QJsonObject SettingsStorage::readDefault(const QString& path) {
  QFile defaults_f(path);
  defaults_f.open(QIODevice::ReadOnly);

  QJsonDocument defaults = QJsonDocument::fromJson(defaults_f.readAll());
  Q_ASSERT(defaults.isObject());

  return defaults.object();
}

QJsonDocument SettingsStorage::readConfig(const QString& source) {
  QFile config_file(source);

  qCDebug(log_persistent) << "Loading configuration from:" << config_file.fileName();

  if (!config_file.open(QIODevice::ReadOnly)) {
    qCWarning(log_persistent) << "Could not open configuration file:" << config_file.fileName()
                              << "E:" << config_file.errorString();
    return {};
  }

  QJsonParseError error{};
  auto config_doc = QJsonDocument::fromJson(config_file.readAll(), &error);

  if (config_doc.isNull() && error.error != QJsonParseError::NoError) {
    qCWarning(log_persistent) << "Configuration file parse error:" << config_file.fileName()
                              << "E:" << error.errorString() << "at:" << error.offset;
    return {};
  }
  return config_doc;
}

void SettingsStorage::writeConfig(const QJsonDocument& doc, const QString& target) {
  QSaveFile config_file(target);

  qCDebug(log_persistent) << "Saving configuration to:" << config_file.fileName();

  if (!config_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    qCWarning(log_persistent) << "Could not open configuration file for write:"
                              << config_file.fileName() << "E:" << config_file.errorString();

  auto json_doc = doc.toJson(QJsonDocument::Indented);

  if (config_file.write(json_doc) != json_doc.size())
    qCWarning(log_persistent) << "Could not write into configuration file:"
                              << config_file.fileName() << "E:" << config_file.errorString();

  if (config_file.isOpen() && config_file.commit())
    qCInfo(log_persistent) << "Saved configuration to:" << config_file.fileName();
  else
    qCWarning(log_persistent) << "Could not save configuration to:" << config_file.fileName();
}

}  // namespace librevault
