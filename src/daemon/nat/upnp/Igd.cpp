/* Copyright (C) 2020 Alexander Shishenko <alex@shishenko.com>
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

#include "Igd.h"

#include <QTimer>
#include <QUrl>
#include <QtCore/QEventLoop>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QNetworkReply>
#include <utility>

#include "control/Config.h"
#include "nat/UPnPService.h"
#include "util/log.h"

// static const QString IGD_IFACE_CIF = QStringLiteral("urn:schemas-upnp-org:service:WANCommonInterfaceConfig:");
static const QString IGD_IFACE_IP = QStringLiteral("urn:schemas-upnp-org:service:WANIPConnection:");
// static const QString IGD_IFACE_IPV6 = QStringLiteral("urn:schemas-upnp-org:service:WANIPv6FirewallControl:");
static const QString IGD_IFACE_PPP = QStringLiteral("urn:schemas-upnp-org:service:WANPPPConnection:");

// static const QStringList IGD_IFACE_URN = {
//    IGD_IFACE_IP, IGD_IFACE_PPP
//};

QByteArray makeSoapHeader(const QString& service_type, const QString& method = "AddPortMapping") {
  return QString("\"%1#%2\"").arg(service_type, method).toUtf8();
}

QLatin1String getProtocolLiteral(QAbstractSocket::SocketType protocol) {
  switch (protocol) {
    case QAbstractSocket::TcpSocket:
      return QLatin1String("TCP");
    case QAbstractSocket::UdpSocket:
      return QLatin1String("UDP");
    default:
      return QLatin1String("");
  }
}

namespace librevault::upnp {

Igd::Igd(QNetworkAccessManager* nam, QObject* parent) : QObject(parent), nam_(nam) {}

void Igd::fetchIgdDescription() {
  auto reply = nam_->get(QNetworkRequest(gateway_desc));
  connect(reply, &QNetworkReply::finished, this, [=, this] {
    QDomDocument metadata;
    metadata.setContent(reply->readAll());

    QString url_base = metadata.elementsByTagName("URLBase").at(0).toElement().text();

    auto services = metadata.elementsByTagName("service");
    for (int i = 0; i < services.size(); i++) {
      auto service_type = services.at(i).firstChildElement("serviceType").text();
      auto control_url = services.at(i).firstChildElement("controlURL").text();

      QUrl control_url_abs = url_base + control_url;

      if (service_type.startsWith(IGD_IFACE_IP))
        ip.append({service_type, control_url_abs});
      else if (service_type.startsWith(IGD_IFACE_PPP))
        ppp.append({service_type, control_url_abs});
    }
    readiness = true;
    emit ready();
  });
}

QNetworkReply* Igd::sendUpnpAction(const UpnpIgdService& service, const QString& action, const QByteArray& message) {
  QNetworkRequest request(service.control_url);
  request.setRawHeader("SOAPAction", makeSoapHeader(service.urn, action));
  request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "text/xml");

  qCDebug(log_upnp) << "SENT:" << message;
  return nam_->post(request, message);
}

QByteArray Igd::constructAddPortMapping(const UpnpIgdService& service, const MappingRequest& request) {
  static const QString IGD_ADDPORTMAPPING = QStringLiteral(
      R"(<?xml version="1.0"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"
            s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
    <s:Body>
        <u:AddPortMapping xmlns:u="{urn}">
            <NewRemoteHost></NewRemoteHost>
            <NewExternalPort>{external_port}</NewExternalPort>
            <NewProtocol>{protocol}</NewProtocol>
            <NewInternalPort>{internal_port}</NewInternalPort>
            <NewInternalClient>{internal_client}</NewInternalClient>
            <NewEnabled>1</NewEnabled>
            <NewPortMappingDescription>{description}</NewPortMappingDescription>
            <NewLeaseDuration>0</NewLeaseDuration>
        </u:AddPortMapping>
    </s:Body>
</s:Envelope>
)");
  QString message = IGD_ADDPORTMAPPING;
  return message.replace("{urn}", service.urn)
      .replace("{external_port}", QString::number(request.port))
      .replace("{protocol}", getProtocolLiteral(request.protocol))
      .replace("{internal_port}", QString::number(request.port))
      .replace("{internal_client}", local_addr.toString())
      .replace("{description}", request.description)
      .toUtf8();
}

QByteArray Igd::constructDeletePortMapping(const UpnpIgdService& service, const MappingRequest& request) {
  static const QString IGD_DELETEPORTMAPPING = QStringLiteral(
      R"(<?xml version="1.0"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"
            s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
    <s:Body>
        <u:DeletePortMapping xmlns:u="{urn}">
            <NewRemoteHost></NewRemoteHost>
            <NewExternalPort>{external_port}</NewExternalPort>
            <NewProtocol>{protocol}</NewProtocol>
        </u:DeletePortMapping>
    </s:Body>
</s:Envelope>
)");
  QString message = IGD_DELETEPORTMAPPING;
  return message.replace("{urn}", service.urn)
      .replace("{external_port}", QString::number(request.port))
      .replace("{protocol}", getProtocolLiteral(request.protocol))
      .toUtf8();
}

void Igd::sendAddPortMapping(const MappingRequest& request) {
  for (const auto& service : (ip + ppp)) {
    auto reply = sendUpnpAction(service, "AddPortMapping", constructAddPortMapping(service, request));
    connect(reply, &QNetworkReply::finished, this, [=, this] { emit portMapped({request.id, request.port}); });
  }
}

void Igd::sendDeletePortMapping(const MappingRequest& request) {
  for (const auto& service : (ip + ppp))
    sendUpnpAction(service, "DeletePortMapping", constructDeletePortMapping(service, request));
}

}  // namespace librevault::upnp
