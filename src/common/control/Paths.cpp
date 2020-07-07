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
