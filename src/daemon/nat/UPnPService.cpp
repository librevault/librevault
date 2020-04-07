/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "UPnPService.h"

#include <QTimer>
#include <QUrl>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QNetworkReply>
#include <utility>

#include "control/Config.h"
#include "util/log.h"

Q_LOGGING_CATEGORY(log_upnp, "upnp")

namespace librevault {

// static const QString IGD_IFACE_CIF = QStringLiteral("urn:schemas-upnp-org:service:WANCommonInterfaceConfig:");
static const QString IGD_IFACE_IP = QStringLiteral("urn:schemas-upnp-org:service:WANIPConnection:");
// static const QString IGD_IFACE_IPV6 = QStringLiteral("urn:schemas-upnp-org:service:WANIPv6FirewallControl:");
static const QString IGD_IFACE_PPP = QStringLiteral("urn:schemas-upnp-org:service:WANPPPConnection:");

// static const QStringList IGD_IFACE_URN = {
//    IGD_IFACE_IP, IGD_IFACE_PPP
//};

const char* get_literal_protocol(int protocol) { return protocol == QAbstractSocket::TcpSocket ? "TCP" : "UDP"; }
QByteArray makeSoapHeader(const QString& service_type, const QString& method = "AddPortMapping") {
  return QString("\"%1#%2\"").arg(service_type, method).toUtf8();
}

UPnPService::UPnPService(PortMappingService& parent) : PortMappingSubService(parent) {
  socket_ = new QUdpSocket(this);
  nam_ = new QNetworkAccessManager(this);

  connect(socket_, &QUdpSocket::readyRead, this, &UPnPService::handleDatagram);
  connect(this, &UPnPService::foundIgd, this, &UPnPService::handleIgd);
}
UPnPService::~UPnPService() { stop(); }

bool UPnPService::is_config_enabled() { return Config::get()->getGlobal("upnp_enabled").toBool(); }

void UPnPService::start() {
  if (!is_config_enabled()) return;
  socket_->bind();

  sendSearchPacket();

  add_existing();
}

void UPnPService::sendSearchPacket() {
  const auto IGD_SEARCH = QByteArrayLiteral(
      "M-SEARCH * HTTP/1.1\r\n"
      "HOST: 239.255.255.250:1900\r\n"
      "ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
      "MAN: \"ssdp:discover\"\r\n"
      "MX: 2\r\n\r\n");
  socket_->writeDatagram(IGD_SEARCH, QHostAddress("239.255.255.250"), 1900);
}

void UPnPService::handleDatagram() {
  while (socket_->hasPendingDatagrams()) {
    QNetworkDatagram datagram = socket_->receiveDatagram();
    for (const auto& line : QString::fromUtf8(datagram.data()).split("\r\n")) {
      auto kv = line.split(": ");
      if (kv.first().toLower() == "location") emit foundIgd(QHostAddress(datagram.senderAddress()), QUrl(kv.last()));
    }
  }
}

void UPnPService::handleIgd(const QHostAddress& addr, const QUrl& url) {
  qCDebug(log_upnp) << "Found IGD:" << url;
  auto reply = nam_->get(QNetworkRequest(url));
  connect(reply, &QNetworkReply::finished, this, [=, this] {
    QDomDocument metadata;
    metadata.setContent(reply->readAll());

    QString url_base = metadata.elementsByTagName("URLBase").at(0).toElement().text();

    auto services = metadata.elementsByTagName("service");
    for (int i = 0; i < services.size(); i++) {
      auto service_type = services.at(i).firstChildElement("serviceType").text();
      auto control_url = services.at(i).firstChildElement("controlURL").text();

      if (service_type.startsWith(IGD_IFACE_IP) || service_type.startsWith(IGD_IFACE_PPP))
        igd_[addr][service_type].append(QUrl(url_base + control_url));
    }
    reply->deleteLater();
    qCDebug(log_upnp) << "Found IGD services:" << igd_;
    add_existing();
  });
}

void UPnPService::sendMapRequest(MappingDescriptor descriptor, const QString& description) {}

void UPnPService::stop() {}

void UPnPService::map(const QString& id, MappingDescriptor descriptor, const QString& description) {
  mappings_.erase(id);
  mappings_[id] = std::make_shared<UPnPMapping>(*this, id, descriptor, description);

  for (const auto& igd_device : igd_.values()) {
    for (const auto& igd_servicetype : igd_device.keys()) {
      QString IGD_SOAP =
          "<?xml version=\"1.0\"?>\r\n"
          "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
          "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:AddPortMapping "
          "xmlns:u=\"" +
          igd_servicetype +
          "\"><NewRemoteHost></"
          "NewRemoteHost><NewExternalPort>" +
          QString::number(descriptor.port) + "</NewExternalPort><NewProtocol>" +
          get_literal_protocol(descriptor.protocol) + "</NewProtocol><NewInternalPort>" +
          QString::number(descriptor.port) +
          "</"
          "NewInternalPort><NewInternalClient>172.18.0.100</NewInternalClient><NewEnabled>1</"
          "NewEnabled><NewPortMappingDescription>" +
          description +
          "</NewPortMappingDescription><NewLeaseDuration>0</NewLeaseDuration></u:AddPortMapping></s:Body></"
          "s:Envelope>\r\n";
      for (const auto& igd_url : igd_device[igd_servicetype]) {
        QNetworkRequest request(igd_url);
        request.setRawHeader("SOAPAction", makeSoapHeader(igd_servicetype, "AddPortMapping"));
        request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "text/xml");

        auto reply = nam_->post(request, IGD_SOAP.toUtf8());
        connect(reply, &QNetworkReply::finished, this,
                [=, this] { qCDebug(log_upnp) << "Shit happened:" << reply->readAll(); });
      }
    }
  }
}

void UPnPService::unmap(const QString& id) { mappings_.erase(id); }

UPnPMapping::UPnPMapping(UPnPService& parent, QString id, MappingDescriptor descriptor, const QString& description)
    : parent_(parent), id_(std::move(id)), descriptor_(descriptor) {}

UPnPMapping::~UPnPMapping() {}

}  // namespace librevault
