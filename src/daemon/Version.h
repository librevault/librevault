#pragma once
#include <QString>

namespace librevault {

class Version {
 public:
  Version();

  QString name() const { return name_; }
  QString versionString() const { return version_string_; }
  QString userAgent() const { return name() + "/" + version_string_; }

  static Version current() {
    static Version current;
    return current;
  }

 protected:
  const QString name_;
  const QString version_string_;
};

}  // namespace librevault
