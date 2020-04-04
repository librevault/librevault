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
#include "FolderSettings.h"
#include <QJsonArray>
#include <QVariant>
#include <QDebug>

namespace librevault {

template <class Value>
void packValue(QJsonValueRef j, const Value& val) {
  j = QJsonValue::fromVariant(val);
}

template <>
void packValue(QJsonValueRef j, const std::chrono::seconds& val) {
  j = std::chrono::duration<double>(val).count();
}

template <>
void packValue(QJsonValueRef j, const Secret& val) {
  j = val.toString();
}

template <>
void packValue(QJsonValueRef j, const QList<QUrl>& val) {
  QStringList strlist;
  for (const auto& url : val) strlist << url.toString();
  j = QJsonArray::fromStringList(strlist);
}

template <class Value>
void unpackValue(Value& val, const QJsonValue& j) {
  val = qvariant_cast<Value>(j.toVariant());
}

template <>
void unpackValue(std::chrono::seconds& val, const QJsonValue& j) {
  val =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::duration<double>(j.toDouble()));
}

template <>
void unpackValue(Secret& val, const QJsonValue& j) {
  val = Secret(j.toString());
}

}  // namespace librevault

namespace librevault::models {

FolderSettings::FolderSettings(const QJsonObject& j) {
  qDebug() << j["secret"];
  unpackValue(secret, j["secret"]);
  unpackValue(path, j["path"]);
  unpackValue(system_path, j["system_path"]);
  unpackValue(preserve_unix_attrib, j["preserve_unix_attrib"]);
  unpackValue(preserve_windows_attrib, j["preserve_windows_attrib"]);
  unpackValue(preserve_symlinks, j["preserve_symlinks"]);
  unpackValue(full_rescan_interval, j["full_rescan_interval"]);
  unpackValue(nodes, j["nodes"]);
  unpackValue(mainline_dht_enabled, j["mainline_dht_enabled"]);
}

QJsonObject FolderSettings::toJson() const {
  QJsonObject j;

  packValue(j["secret"], secret);
  packValue(j["path"], path);
  packValue(j["system_path"], system_path);
  packValue(j["preserve_unix_attrib"], preserve_unix_attrib);
  packValue(j["preserve_windows_attrib"], preserve_windows_attrib);
  packValue(j["preserve_symlinks"], preserve_symlinks);
  packValue(j["full_rescan_interval"], full_rescan_interval);
  packValue(j["nodes"], nodes);
  packValue(j["mainline_dht_enabled"], mainline_dht_enabled);

  return j;
}

}  // namespace librevault::models
