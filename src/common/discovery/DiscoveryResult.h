#pragma once
#include <QByteArray>
#include <QHostAddress>
#include <QString>
#include <QUrl>

#include "util/Endpoint.h"

namespace librevault {

class DiscoveryResult {
 public:
  QString source;
  QUrl url;
  Endpoint endpoint;
  QByteArray digest;
};

}  // namespace librevault
