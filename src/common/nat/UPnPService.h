#pragma once
#include <QLoggingCategory>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QUdpSocket>
#include <QtXml/QDomDocument>
#include <array>
#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>
#include <mutex>

#include "PortMappingSubService.h"
#include "nat/upnp/Igd.h"

Q_DECLARE_LOGGING_CATEGORY(log_upnp)

namespace librevault {

class UPnPService : public PortMappingSubService {
  Q_OBJECT
 public:
  UPnPService(QNetworkAccessManager* nam, PortMappingService& parent);
  ~UPnPService() override;

  void start();
  void stop();

  void map(const MappingRequest& request);
  void unmap(const QString& id);

 protected:
  QHash<QString, MappingRequest> mappings_;
  QNetworkAccessManager* nam_;
  QUdpSocket* socket_;
  QHash<QString, upnp::Igd*> igd_;

  void sendSsdpSearch();
  void handleDatagram();

  [[nodiscard]] QList<upnp::Igd*> activeIgd() const;
};

}  // namespace librevault
