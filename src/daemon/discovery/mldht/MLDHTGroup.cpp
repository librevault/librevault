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
#include "MLDHTGroup.h"

#include <dht.h>

#include <boost/asio/ip/udp.hpp>

#include "MLDHTProvider.h"
#include "folder/FolderGroup.h"
#include "util/Endpoint.h"

namespace librevault {

MLDHTGroup::MLDHTGroup(MLDHTProvider* provider, FolderGroup* fgroup)
    : provider_(provider), info_hash_(btcompat::getInfoHash(fgroup->folderid())), folderid_(fgroup->folderid()) {
  timer_ = new QTimer(this);

  timer_->setInterval(30 * 1000);

  connect(provider_, &MLDHTProvider::eventReceived, this, &MLDHTGroup::handleEvent, Qt::QueuedConnection);
  connect(timer_, &QTimer::timeout, this, [this] { startSearch(AF_INET); });
  connect(timer_, &QTimer::timeout, this, [this] { startSearch(AF_INET6); });
}

void MLDHTGroup::setEnabled(bool enable) {
  if (enable && !enabled_) {
    enabled_ = true;
    timer_->start();
  } else if (!enable && enabled_) {
    timer_->stop();
    enabled_ = false;
  }
}

void MLDHTGroup::startSearch(int af) {
  bool announce = true;

  QByteArray info_hash((const char*)info_hash_.data(), info_hash_.size());

  qCDebug(log_dht) << "Starting" << (af == AF_INET6 ? "IPv6" : "IPv4") << (announce ? "announce" : "search")
                   << "for:" << info_hash.toHex() << (announce ? "on port:" : "")
                   << (announce ? QString::number(provider_->getExternalPort()) : QString());

  dht_search(info_hash_.data(), announce ? provider_->getExternalPort() : 0, af, lv_dht_callback_glue, provider_);
}

void MLDHTGroup::handleEvent(int event, btcompat::info_hash ih, QByteArray values) {
  if (!enabled_) return;
  if (event == DHT_EVENT_VALUES || event == DHT_EVENT_VALUES6) {
    EndpointList endpoints;

    if (event == DHT_EVENT_VALUES)
      endpoints = Endpoint::fromPackedList4(values);
    else if (event == DHT_EVENT_VALUES6)
      endpoints = Endpoint::fromPackedList6(values);

    for (auto& endpoint : endpoints) {
      DiscoveryResult result;
      result.source = "DHT";
      result.endpoint = endpoint;
      emit provider_->discovered(folderid_, result);
    }
  }
}

}  // namespace librevault
