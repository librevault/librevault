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
  Igd(QObject* parent);

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
  QNetworkAccessManager* nam_ = nullptr;

  QNetworkReply* sendUpnpAction(const UpnpIgdService& service, const QString& action, const QByteArray& message);

  QByteArray constructAddPortMapping(const UpnpIgdService& service, const MappingRequest& request);
  QByteArray constructDeletePortMapping(const UpnpIgdService& service, const MappingRequest& request);
  bool readiness = false;
};

}  // namespace librevault::upnp
