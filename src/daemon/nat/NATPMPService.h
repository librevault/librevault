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

  void map(const QString &id, MappingDescriptor descriptor,
           const QString &description);
  void unmap(const QString &id);

 protected:
  bool is_config_enabled();
  natpmp_t natpmp;

  std::map<QString, std::unique_ptr<NATPMPMapping>> mappings_;
};

class NATPMPMapping : public QObject {
  Q_OBJECT
 public:
  NATPMPMapping(NATPMPService& parent, QString id,
                MappingDescriptor descriptor);
  ~NATPMPMapping();

 private:
  NATPMPService& parent_;
  QString id_;
  MappingDescriptor descriptor_;

  QTimer* timer_;

 private slots:
  void sendPeriodicRequest();
  void sendZeroRequest();
};

} /* namespace librevault */
