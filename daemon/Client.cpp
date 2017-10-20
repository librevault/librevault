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
#include "folder/FolderGroup.h"
#include "nodekey/NodeKey.h"
#include "p2p/PeerPool.h"
#include "p2p/PeerServer.h"
#include <discovery/bt/BTProvider.h>
#include <discovery/dht/DHTProvider.h>
#include <discovery/multicast/MulticastProvider.h>
#include <NatPmpService.h>
#include <QTimer>

namespace librevault {

Q_LOGGING_CATEGORY(log_client, "client")

Client::Client(int argc, char** argv) : QCoreApplication(argc, argv) {
  qRegisterMetaType<SignedMeta>("SignedMeta");

  // Initializing components
  node_key_ = new NodeKey(this);
  portmanager_ = new NatPmpService(this);
  peerserver_ = new PeerServer(node_key_, portmanager_, this);

  portmanager_->setEnabled(true);

  /* Connecting signals */
  connect(Config::get(), &Config::folderAdded, this, &Client::initFolder);
  connect(Config::get(), &Config::folderRemoved, this, &Client::deinitFolder);

  QTimer::singleShot(0, this, &Client::initializeAll);
}

Client::~Client() {
  delete peerserver_;
  delete portmanager_;
  delete node_key_;
}

void Client::initializeAll() {
  peerserver_->start();

  // Initialize all existing folders
  for (const QByteArray& folderid : Config::get()->listFolders())
    initFolder(Config::get()->getFolder(folderid));
}

void Client::deinitializeAll() {
  for (const QByteArray& folderid : Config::get()->listFolders())
    deinitFolder(folderid);

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
  discovery_multicast_ = new MulticastProvider(this);
  discovery_multicast_->setAnnouncePort(12345);

  discovery_multicast_->setGroupEndpoint({QHostAddress("239.192.152.144"), 28914});
  discovery_multicast_->setEnabled(true);

  // DHT
  discovery_dht_ = new DHTProvider(this);
  discovery_dht_->setAnnouncePort(12345);
  discovery_dht_->setEnabled(true);

  discovery_dht_->addRouter("router.utorrent.com", 6881);
  discovery_dht_->addRouter("router.bittorrent.com", 6881);
  discovery_dht_->addRouter("dht.transmissionbt.com", 6881);
  discovery_dht_->addRouter("dht.aelitis.com", 6881);
  discovery_dht_->addRouter("dht.libtorrent.org", 25401);

  // BitTorrent
  discovery_bt_ = new BTProvider(this);
  discovery_bt_->setAnnouncePort(12345);
}

void Client::deinitDiscovery() {

}

void Client::initFolder(const FolderParams& params) {
  auto fgroup = new FolderGroup(params, this);
  groups_[params.folderid()] = fgroup;

  auto peer_pool = new PeerPool(params, node_key_, this);
  fgroup->setPeerPool(peer_pool);
  peerserver_->addPeerPool(params.folderid(), peer_pool);

  qCInfo(log_client) << "Folder initialized: " << params.folderid().toHex();
}

void Client::deinitFolder(const QByteArray& folderid) {
  auto fgroup = groups_[folderid];
  groups_.remove(folderid);

  fgroup->deleteLater();
  qCInfo(log_client) << "Folder deinitialized:" << folderid.toHex();
}

} /* namespace librevault */
