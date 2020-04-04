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
#include "Client.h"
#include "control/Config.h"
#include "folder/FolderController.h"
#include "folder/FolderGroup.h"
#include "nodekey/NodeKey.h"
#include "p2p/PeerPool.h"
#include "p2p/PeerServer.h"
#include "webserver/Webserver.h"
#include <NatPmpService.h>
#include <discovery/bt/BTProvider.h>
#include <discovery/dht/DHTProvider.h>
#include <discovery/multicast/MulticastProvider.h>

namespace librevault {

Q_LOGGING_CATEGORY(log_client, "client")

Client::Client(Config* config, int argc, char** argv)
    : QCoreApplication(argc, argv), config_(config) {
  qRegisterMetaType<SignedMeta>("SignedMeta");

  // Initializing components
  node_key_ = new NodeKey(this);
  portmanager_ = new NatPmpService(this);
  mcast_ = new MulticastProvider(this);
  bt_ = new BTProvider(this);
  dht_ = new DHTProvider(this);

  peerserver_ = new PeerServer(node_key_, portmanager_, config_, this);
  folder_controller_ = new FolderController(config_, bt_, mcast_, dht_, peerserver_, node_key_,this);
  webserver_ = new Webserver(config_, this);
  portmanager_->setEnabled(true);

  QTimer::singleShot(0, this, &Client::initializeAll);
}

Client::~Client() {
  delete webserver_;
  delete peerserver_;
  delete portmanager_;
  delete node_key_;
}

void Client::initializeAll() {
  peerserver_->start();
  initDiscovery();
  webserver_->start();
  folder_controller_->loadAll();
}

void Client::deinitializeAll() {
  folder_controller_->unloadAll();
  deinitDiscovery();
  peerserver_->stop();
}

void Client::restart() {
  deinitializeAll();
  qCInfo(log_client) << "Restarting...";
  this->exit(EXIT_RESTART);
}

void Client::shutdown() {
  deinitializeAll();
  qCInfo(log_client) << "Exiting...";
  this->exit();
}

void Client::initDiscovery() {
  QByteArray discovery_id = node_key_->digest();

  // Multicast
  mcast_->setGroupEndpoint({QHostAddress("239.192.152.144"), 28914});
  mcast_->setEnabled(config_->getGlobals()["multicast_enabled"].toBool());

  // DHT
  dht_->setEnabled(config_->getGlobals()["mainline_dht_enabled"].toBool());
  dht_->addRouter("router.utorrent.com", 6881);
  dht_->addRouter("router.bittorrent.com", 6881);
  dht_->addRouter("dht.transmissionbt.com", 6881);
  dht_->addRouter("dht.aelitis.com", 6881);
  dht_->addRouter("dht.libtorrent.org", 25401);

  // BitTorrent
  bt_->setEnabled(config_->getGlobals()["bttracker_enabled"].toBool());
  bt_->setIDPrefix(config_->getGlobals()["bttracker_azureus_id"].toString().toLatin1());

  // common
  mcast_->setAnnouncePort(peerserver_->externalPort());
  dht_->setAnnouncePort(peerserver_->externalPort());
  bt_->setAnnouncePort(peerserver_->externalPort());

  connect(peerserver_, &PeerServer::externalPortChanged, mcast_, &GenericProvider::setAnnouncePort);
  connect(peerserver_, &PeerServer::externalPortChanged, dht_, &GenericProvider::setAnnouncePort);
  connect(peerserver_, &PeerServer::externalPortChanged, bt_, &GenericProvider::setAnnouncePort);
}

void Client::deinitDiscovery() {
  delete bt_;
  delete dht_;
  delete mcast_;
}

}  // namespace librevault
