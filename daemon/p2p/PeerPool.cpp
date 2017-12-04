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
#include "PeerPool.h"
#include "discovery/bt/BTGroup.h"
#include "discovery/dht/DHTGroup.h"
#include "discovery/multicast/MulticastGroup.h"
#include "p2p/Peer.h"
#include "p2p/PeerServer.h"

namespace librevault {

Q_LOGGING_CATEGORY(log_peerpool, "p2p.peerpool")

PeerPool::PeerPool(const FolderSettings& params, NodeKey* node_key, BTProvider* bt, DHTProvider* dht,
    MulticastProvider* multicast, QObject* parent)
    : QObject(parent), params_(params), node_key_(node_key) {
  bt_group_ = new BTGroup(params.folderid(), bt, this);
  bt_group_->setEnabled(true);
  connect(bt_group_, &GenericGroup::discovered, this, &PeerPool::handleDiscovered);

  dht_group_ = new DHTGroup(params.folderid(), dht, this);
  dht_group_->setEnabled(true);
  connect(dht_group_, &GenericGroup::discovered, this, &PeerPool::handleDiscovered);

  multicast_group_ = new MulticastGroup(params.folderid(), multicast, this);
  multicast_group_->setEnabled(true);

  connect(multicast_group_, &GenericGroup::discovered, this, &PeerPool::handleDiscovered);

  qCDebug(log_peerpool) << "Adding" << params_.nodes.size() << "nodes from configuration";
  for (const auto& url : params_.nodes) handleNewUrl(url);
}

PeerPool::~PeerPool() = default;

bool PeerPool::contains(Peer* peer) const {
  return peers_.contains(peer) || digests_.contains(peer->digest()) ||
         endpoints_.contains(peer->endpoint());
}

void PeerPool::handleDiscovered(const Endpoint& endpoint) {
  if (endpoints_.contains(endpoint)) return;

  QUrl ws_url = Peer::makeUrl(endpoint, params_.folderid());
  return handleNewUrl(ws_url);
}

void PeerPool::handleNewUrl(const QUrl& url) {
  qCDebug(log_peerpool) << "Handling an url:" << url;

  Peer* peer = new Peer(params_, node_key_, &bc_all_, &bc_blocks_, this);
  connect(peer, &Peer::handshakeSuccess, this, [=] { handleHandshake(peer); });
  connect(peer, &Peer::disconnected, this, [=] { handleDisconnected(peer); });
  connect(peer, &Peer::connected, this, [=] { handleConnected(peer); });

  peer->open(url);

  peers_.insert(peer);
  endpoints_.insert(peer->endpoint());
}

void PeerPool::handleIncoming(QWebSocket* socket) {
  qCDebug(log_peerpool) << "Handling an incoming connection:"
                        << Endpoint{socket->peerAddress(), socket->peerPort()};

  Peer* peer = new Peer(params_, node_key_, &bc_all_, &bc_blocks_, this);
  connect(peer, &Peer::handshakeSuccess, this, [=] { handleHandshake(peer); });
  connect(peer, &Peer::disconnected, this, [=] { handleDisconnected(peer); });
  connect(peer, &Peer::connected, this, [=] { handleConnected(peer); });

  peer->setConnectedSocket(socket);

  peers_.insert(peer);
  endpoints_.insert(peer->endpoint());
}

void PeerPool::handleHandshake(Peer* peer) {
  peers_ready_.insert(peer);
  emit newValidPeer(peer);
}

void PeerPool::handleDisconnected(Peer* peer) {
  peers_.remove(peer);
  peers_ready_.remove(peer);

  endpoints_.remove(peer->endpoint());
  digests_.remove(peer->digest());

  peer->deleteLater();
}

void PeerPool::handleConnected(Peer* peer) {
  if (digests_.contains(peer->digest())) {
    peer->deleteLater();
    return;
  }

  digests_.insert(peer->digest());
}

} /* namespace librevault */
