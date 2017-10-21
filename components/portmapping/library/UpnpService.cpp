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
#include "UpnpService.h"
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <QList>
#include <QTimer>
#include <cerrno>

namespace librevault {

Q_LOGGING_CATEGORY(log_upnp, "portmapping.upnp")

namespace {
struct DevListWrapper {
  DevListWrapper() {
    int error = UPNPDISCOVER_SUCCESS;
    std::chrono::milliseconds delay(2000);
#if MINIUPNPC_API_VERSION >= 14
    devlist = upnpDiscover(delay.count(), nullptr, nullptr, 0, 0, 2, &error);
#else
    devlist = upnpDiscover(delay.count(), nullptr, nullptr, 0, 0, &error);
#endif
    if (error != UPNPDISCOVER_SUCCESS) {
      throw std::runtime_error(strerror(errno));
    }
  }
  ~DevListWrapper() { freeUPNPDevlist(devlist); }

  UPNPDev* devlist;
};

const char* makeProtocol(QAbstractSocket::SocketType protocol) {
  return protocol == QAbstractSocket::TcpSocket ? "TCP" : "UDP";
}
}  // namespace

UpnpService::UpnpService(QObject* parent) : GenericNatService(parent) {
  upnp_urls = std::make_unique<UPNPUrls>();
  upnp_data = std::make_unique<IGDdatas>();
}

UpnpService::~UpnpService() = default;

PortMapping* UpnpService::createMapping(const MappingRequest& request) {
  return new UpnpPortMapping(request, this);
}

void UpnpService::start() {
  try {
    DevListWrapper devlist;

    error_ = UPNP_GetValidIGD(
        devlist.devlist, upnp_urls.get(), upnp_data.get(), lanaddr.data(), lanaddr.size());
    if (!error_) {
      qCDebug(log_upnp) << "Found IGD:" << upnp_urls->controlURL;
      emit started();
    } else {
      throw std::runtime_error(strerror(errno));
    }
  } catch (const std::exception& e) {
    FreeUPNPUrls(upnp_urls.get());
    qCDebug(log_upnp) << "IGD not found. e:" << e.what();
  }
}

void UpnpService::stop() { FreeUPNPUrls(upnp_urls.get()); }

/* PortMapping */
UpnpPortMapping::UpnpPortMapping(const MappingRequest& request, UpnpService* parent)
    : PortMapping(request, parent, parent) {}

void UpnpPortMapping::start() {
  if (isMapped()) return;

  error_ = UPNP_AddPortMapping(qobject_cast<UpnpService*>(service_)->upnp_urls->controlURL,
      qobject_cast<UpnpService*>(service_)->upnp_data->first.servicetype,
      std::to_string(request_.internal_port).c_str(),
      std::to_string(request_.external_port).c_str(),
      qobject_cast<UpnpService*>(service_)->lanaddr.data(), request_.description.toUtf8(),
      makeProtocol(request_.protocol), nullptr, nullptr);
  if (!error_) {
    actual_external_port_ = request_.internal_port;  // TODO: fix external port
  } else {
    actual_external_port_ = 0;
    qCDebug(log_upnp) << "UPnP port forwarding failed: Error" << error_;
  }

  emit mapped(externalPort(), externalAddress(), expiration());
}

void UpnpPortMapping::stop() {
  if (!isMapped()) return;

  auto err = UPNP_DeletePortMapping(qobject_cast<UpnpService*>(service_)->upnp_urls->controlURL,
      qobject_cast<UpnpService*>(service_)->upnp_data->first.servicetype,
      std::to_string(request_.internal_port).c_str(), makeProtocol(request_.protocol), nullptr);
  if (err)
    qCDebug(log_upnp) << makeProtocol(request_.protocol) << "port" << request_.internal_port
                      << "de-forwarding failed: Error" << err;
}

}  // namespace librevault
