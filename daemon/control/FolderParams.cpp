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
#include "FolderParams.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace librevault {

FolderParams::FolderParams(QJsonObject fconfig) {
  // Necessary
  secret = Secret(fconfig["secret"].toString());
  path = fconfig["path"].toString();

  // Optional
  system_path = fconfig["system_path"].toString();
  index_event_timeout = std::chrono::milliseconds(fconfig["index_event_timeout"].toInt());
  preserve_unix_attrib = fconfig["preserve_unix_attrib"].toBool();
  preserve_windows_attrib = fconfig["preserve_windows_attrib"].toBool();
  preserve_symlinks = fconfig["preserve_symlinks"].toBool();
  full_rescan_interval = std::chrono::seconds(fconfig["full_rescan_interval"].toInt());

  for (const auto& node : fconfig["nodes"].toArray()) nodes.push_back(node.toString());

  mainline_dht_enabled = fconfig["mainline_dht_enabled"].toBool();
}

}  // namespace librevault
