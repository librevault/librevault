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
//#include <miniupnpc/miniupnpc.h>
#include "GenericNatService.h"
#include <QString>
#include <array>
#include <memory>
#include <set>

struct UPNPUrls;
struct IGDdatas;
struct UPNPDev;

namespace librevault {

Q_DECLARE_LOGGING_CATEGORY(log_upnp)

class UpnpPortMapping;
class UpnpService : public GenericNatService {
  Q_OBJECT

 public:
  explicit UpnpService(QObject* parent);
  virtual ~UpnpService();

  bool isReady() override;
  PortMapping* createMapping(const MappingRequest& request) override;

 protected:
  friend class UpnpPortMapping;

  // Config values
  std::unique_ptr<UPNPUrls> upnp_urls;
  std::unique_ptr<IGDdatas> upnp_data;
  std::array<char, 16> lanaddr;

  bool ready_ = false;

  Q_SLOT void startup();
};

class UpnpPortMapping : public PortMapping {
  Q_OBJECT

 public:
  UpnpPortMapping(const MappingRequest& request, UpnpService* parent);
  virtual ~UpnpPortMapping();

  Q_SLOT void refresh() override;

  bool isMapped() const override;

 private:
  UpnpService* service;

  Q_SLOT void serviceReady();
  Q_SLOT void teardown();
};

}  // namespace librevault
