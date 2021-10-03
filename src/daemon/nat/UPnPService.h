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

#include "PortMappingSubService.h"
#include "nat/upnp/Igd.h"

Q_DECLARE_LOGGING_CATEGORY(log_upnp)

namespace librevault {

class UPnPService : public PortMappingSubService {
  Q_OBJECT
 public:
  UPnPService(QNetworkAccessManager* nam, PortMappingService& parent);
  ~UPnPService();

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
