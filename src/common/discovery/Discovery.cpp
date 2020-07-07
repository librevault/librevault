#include "Discovery.h"

#include "control/StateCollector.h"
#include "discovery/StaticGroup.h"
#include "discovery/mldht/MLDHTGroup.h"
#include "discovery/mldht/MLDHTProvider.h"
#include "discovery/multicast/MulticastGroup.h"
#include "discovery/multicast/MulticastProvider.h"
#include "discovery/tracker/TrackerProvider.h"
#include "folder/FolderGroup.h"
#include "nodekey/NodeKey.h"

namespace librevault {

Discovery::Discovery(const QByteArray& node_id, PortMappingService* port_mapping, StateCollector* state_collector,
                     QNetworkAccessManager* nam, QObject* parent)
    : QObject(parent) {
  multicast_ = new MulticastProvider(node_id, this);
  mldht_ = new MLDHTProvider(port_mapping, this);
  tracker_ = new TrackerProvider(node_id, port_mapping, nam, this);

  connect(mldht_, &MLDHTProvider::nodeCountChanged, state_collector,
          [=, this](int count) { state_collector->global_state_set("dht_nodes_count", count); });

  connect(multicast_, &MulticastProvider::discovered, this, &Discovery::discovered);
  connect(mldht_, &MLDHTProvider::discovered, this, &Discovery::discovered);

  connect(this, &Discovery::discovered, mldht_,
          [=, this](QByteArray, DiscoveryResult result) { mldht_->addNode(result.endpoint); });
}

Discovery::~Discovery() {}

void Discovery::addGroup(FolderGroup* fgroup) {
  auto groupid = fgroup->folderid();
  auto mldht_group = new MLDHTGroup(mldht_, groupid, this);
  auto multicast_group = new MulticastGroup(multicast_, groupid, this);
  auto static_group = new StaticGroup(fgroup, this);
  tracker_->addGroup(groupid);

  connect(static_group, &StaticGroup::discovered, this,
          [=, this](const DiscoveryResult& result) { emit discovered(groupid, result); });

  mldht_group->setEnabled(true);
  multicast_group->setEnabled(true);
  static_group->setEnabled(true);
}

void Discovery::removeGroup(const QByteArray& groupid) {
  // TODO
}

}  // namespace librevault
