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
