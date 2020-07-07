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
