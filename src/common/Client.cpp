#include "Client.h"

#include "control/Config.h"
#include "control/StateCollector.h"
#include "discovery/Discovery.h"
#include "folder/FolderGroup.h"
#include "folder/FolderService.h"
#include "nat/PortMappingService.h"
#include "nodekey/NodeKey.h"
#include "p2p/P2PProvider.h"

namespace librevault {

Client::Client(QObject* parent) : QObject(parent) {
  // Initializing components
  nam_ = new QNetworkAccessManager(this);
  //  nam_->setAutoDeleteReplies(true);
  //  nam_->setStrictTransportSecurityEnabled(true);
  //  nam_->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

  state_collector_ = new StateCollector(this);
  node_key_ = new NodeKey(this);
  portmanager_ = new PortMappingService(nam_, this);
  discovery_ = new Discovery(node_key_->digest(), portmanager_, state_collector_, nam_, this);
  folder_service_ = new FolderService(state_collector_, this);
  p2p_provider_ = new P2PProvider(node_key_, portmanager_, folder_service_, this);

  /* Connecting signals */
  connect(state_collector_, &StateCollector::globalStateChanged, this, &Client::globalStateChanged);
  connect(state_collector_, &StateCollector::folderStateChanged, this, &Client::folderStateChanged);
  connect(discovery_, &Discovery::discovered, p2p_provider_, &P2PProvider::handleDiscovered);
  connect(folder_service_, &FolderService::folderAdded, discovery_, &Discovery::addGroup);

  connect(folder_service_, &FolderService::folderAdded, this,
          [this](FolderGroup* group) { folderAdded(group->folderid()); });
  connect(folder_service_, &FolderService::folderRemoved, this,
          [this](FolderGroup* group) { folderRemoved(group->folderid()); });
}

Client::~Client() = default;

void Client::run() { folder_service_->run(); }

}  // namespace librevault
