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
#include "Paths.h"

#include <QDir>

namespace librevault {

Paths::Paths(QString appdata_path)
    : appdata_path(appdata_path.isEmpty() ? default_appdata_path() : appdata_path),
      client_config_path(this->appdata_path + "/globals.json"),
      folders_config_path(this->appdata_path + "/folders.json"),
      log_path(this->appdata_path + "/librevault.log"),
      key_path(this->appdata_path + "/key.pem"),
      cert_path(this->appdata_path + "/cert.pem"),
      dht_session_path(this->appdata_path + "/session_dht.json") {
  QDir().mkpath(this->appdata_path);
}

Paths* Paths::instance_ = nullptr;

}  // namespace librevault
