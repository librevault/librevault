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
  virtual ~PortMappingService();

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
