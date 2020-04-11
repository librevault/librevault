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
#include "Discovery.h"

#include "control/StateCollector.h"
#include "discovery/StaticGroup.h"
#include "discovery/mldht/MLDHTGroup.h"
#include "discovery/mldht/MLDHTProvider.h"
#include "discovery/multicast/MulticastGroup.h"
#include "discovery/multicast/MulticastProvider.h"
#include "folder/FolderGroup.h"

namespace librevault {

Discovery::Discovery(NodeKey* node_key, PortMappingService* port_mapping, StateCollector* state_collector,
                     QObject* parent)
    : QObject(parent) {
  multicast_ = new MulticastProvider(node_key, this);
  mldht_ = new MLDHTProvider(port_mapping, this);

  connect(mldht_, &MLDHTProvider::nodeCountChanged, state_collector,
          [=, this](int count) { state_collector->global_state_set("dht_nodes_count", count); });

  connect(multicast_, &MulticastProvider::discovered, this, &Discovery::discovered);
  connect(mldht_, &MLDHTProvider::discovered, this, &Discovery::discovered);

  connect(this, &Discovery::discovered, mldht_,
          [=, this](QByteArray, DiscoveryResult result) { mldht_->addNode(result.endpoint); });
}

Discovery::~Discovery() {}

void Discovery::addGroup(FolderGroup* fgroup) {
  auto mldht_group = new MLDHTGroup(mldht_, fgroup);
  auto multicast_group = new MulticastGroup(multicast_, fgroup);
  auto static_group = new StaticGroup(fgroup);

  connect(static_group, &StaticGroup::discovered, this,
          [=, this](const DiscoveryResult& result) { emit discovered(fgroup->folderid(), result); });

  mldht_group->setEnabled(true);
  multicast_group->setEnabled(true);
  static_group->setEnabled(true);
}

}  // namespace librevault
