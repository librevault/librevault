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
#include <QString>

namespace librevault {

class Paths {
 public:
  static Paths* get(QString appdata_path = QString()) {
    if (!instance_) instance_ = new Paths(appdata_path);
    return instance_;
  }
  static void deinit() {
    delete instance_;
    instance_ = nullptr;
  }

  const QString appdata_path, client_config_path, folders_config_path, log_path, key_path, cert_path, dht_session_path;

 private:
  Paths(QString appdata_path);
  QString default_appdata_path();

  static Paths* instance_;
};

}  // namespace librevault
