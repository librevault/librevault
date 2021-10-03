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
#include <shlobj.h>

#include "Paths.h"
#include "Version.h"

namespace librevault {

QString Paths::default_appdata_path() {
  PWSTR appdata_path;  // PWSTR is wchar_t*
  SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appdata_path);
  QString folder_path = QString::fromWCharArray(appdata_path) + "/Librevault";
  CoTaskMemFree(appdata_path);

  return folder_path;
}

}  // namespace librevault
