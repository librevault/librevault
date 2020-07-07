#pragma once
#include <QAbstractSocket>
#include <QLoggingCategory>
#include <QtNetwork/QNetworkAccessManager>

#include "util/log.h"

Q_DECLARE_LOGGING_CATEGORY(log_portmapping)

namespace librevault {

class NATPMPService;
class UPnPService;
class PortMappingSubService;

struct MappingRequest {
  QString id;
  uint16_t port = 0;
  QAbstractSocket::SocketType protocol;
  QString description;
};

struct MappingResult {
  QString id;
  uint16_t external_port = 0;
};

class PortMappingService : public QObject {
  Q_OBJECT
  friend class PortMappingSubService;

 public:
  PortMappingService(QNetworkAccessManager* nam, QObject* parent);
  ~PortMappingService() override;

  void map(const MappingRequest& request);
  void unmap(const QString& id);
  uint16_t mappedPortOrOriginal(const QString& id);

 private:
  struct Mapping {
    MappingRequest request;
    MappingResult result;
  };
  QHash<QString, Mapping> mappings_;

  NATPMPService* natpmp_;
  UPnPService* upnp_;

  void add_existing_mappings(PortMappingSubService* subservice);
  void portCallback(const MappingResult& result);
};

}  // namespace librevault
