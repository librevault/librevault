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

#include "nat/PortMappingSubService.h"

Q_DECLARE_LOGGING_CATEGORY(log_upnp)

namespace librevault::upnp {

class Igd : public QObject {
  Q_OBJECT
 public:
  Igd(QNetworkAccessManager* nam, QObject* parent);

  struct UpnpIgdService {
    QString urn;
    QUrl control_url;
  };
  QHostAddress local_addr, igd_addr;
  QList<UpnpIgdService> ip, ppp, ipv6;
  QUrl gateway_desc;

  void fetchIgdDescription();

  void sendAddPortMapping(const MappingRequest& request);
  void sendDeletePortMapping(const MappingRequest& request);

  Q_SIGNAL void ready();
  bool isReady() { return readiness; }

  Q_SIGNAL void portMapped(const MappingResult& result);

 private:
  QNetworkAccessManager* nam_;

  QNetworkReply* sendUpnpAction(const UpnpIgdService& service, const QString& action, const QByteArray& message);

  QByteArray constructAddPortMapping(const UpnpIgdService& service, const MappingRequest& request);
  QByteArray constructDeletePortMapping(const UpnpIgdService& service, const MappingRequest& request);
  bool readiness = false;
};

}  // namespace librevault::upnp
