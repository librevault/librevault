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
#include <natpmp.h>

#include <QLoggingCategory>
#include <QTimer>

#include "PortMappingSubService.h"
#include "util/log.h"

Q_DECLARE_LOGGING_CATEGORY(log_natpmp)

namespace librevault {

class P2PProvider;
class NATPMPMapping;
class NATPMPService : public PortMappingSubService {
  friend class NATPMPMapping;

 public:
  NATPMPService(PortMappingService& parent);
  virtual ~NATPMPService();

  void start();
  void stop();

  void map(const MappingRequest& request);
  void unmap(const QString& id);

 protected:
  bool active_;
  natpmp_t natpmp;

  QHash<QString, MappingRequest> requests_;
  std::map<QString, std::unique_ptr<NATPMPMapping>> mappings_;
};

class NATPMPMapping : public QObject {
  Q_OBJECT
 public:
  NATPMPMapping(NATPMPService& parent, MappingRequest request);
  ~NATPMPMapping();

 private:
  NATPMPService& parent_;
  MappingRequest request_;

  QTimer* timer_;

 private slots:
  void sendPeriodicRequest();
  void sendZeroRequest();
};

}  // namespace librevault
