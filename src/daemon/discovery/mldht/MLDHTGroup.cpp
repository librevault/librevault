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
#include "MLDHTGroup.h"

#include <dht.h>
#include "crypto/Hex.h"
#include "util/Endpoint.h"

#include "MLDHTProvider.h"
#include "dht_glue.h"
#include "folder/FolderGroup.h"

namespace librevault {

MLDHTGroup::MLDHTGroup(MLDHTProvider* provider, FolderGroup* fgroup)
    : provider_(provider), info_hash_(btcompat::getInfoHash(fgroup->folderid())), folderid_(fgroup->folderid()) {
  timer_ = new QTimer(this);

  timer_->setInterval(30 * 1000);

  connect(provider_, &MLDHTProvider::eventReceived, this, &MLDHTGroup::handleEvent, Qt::QueuedConnection);
  connect(timer_, &QTimer::timeout, this, [this] { start_search(AF_INET); });
  connect(timer_, &QTimer::timeout, this, [this] { start_search(AF_INET6); });
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

void MLDHTGroup::start_search(int af) {
  bool announce = true;

  qCDebug(log_dht) << "Starting" << (af == AF_INET6 ? "IPv6" : "IPv4") << (announce ? "announce" : "search")
                   << "for: " << crypto::Hex().to_string(info_hash_).c_str() << (announce ? "on port:" : "")
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
