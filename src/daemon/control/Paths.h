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
