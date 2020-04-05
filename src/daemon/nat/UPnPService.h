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
#include <QLoggingCategory>
#include <array>
#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>
#include <mutex>

#include "PortMappingSubService.h"

struct UPNPUrls;
struct IGDdatas;
struct UPNPDev;

Q_DECLARE_LOGGING_CATEGORY(log_upnp)

namespace librevault {

class UPnPMapping;

class UPnPService : public PortMappingSubService {
 public:
  UPnPService(PortMappingService& parent);
  ~UPnPService();

  void start();
  void stop();

  void map(const QString &id, MappingDescriptor descriptor,
           const QString &description);
  void unmap(const QString &id);

 protected:
  friend class UPnPMapping;
  std::map<QString, std::shared_ptr<UPnPMapping>> mappings_;

  // Config values
  std::unique_ptr<UPNPUrls> upnp_urls;
  std::unique_ptr<IGDdatas> upnp_data;
  std::array<char, 16> lanaddr;

  bool is_config_enabled();
};

class UPnPMapping {
 public:
  UPnPMapping(UPnPService& parent, QString id, MappingDescriptor descriptor,
              QString description);
  virtual ~UPnPMapping();

 private:
  UPnPService& parent_;
  QString id_;
  MappingDescriptor descriptor_;
};

} /* namespace librevault */
