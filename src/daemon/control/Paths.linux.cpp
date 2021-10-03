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
#include <pwd.h>
#include <unistd.h>

#include "Paths.h"

namespace librevault {

QString Paths::default_appdata_path() {
  if (char* xdg_ptr = getenv("XDG_CONFIG_HOME")) return QString::fromUtf8(xdg_ptr) + "/Librevault";
  if (char* home_ptr = getenv("HOME")) return QString::fromUtf8(home_ptr) + "/.config/Librevault";
  if (char* home_ptr = getpwuid(getuid())->pw_dir) return QString::fromUtf8(home_ptr) + "/.config/Librevault";
  return QStringLiteral("/etc/xdg") + "/Librevault";
}

}  // namespace librevault
