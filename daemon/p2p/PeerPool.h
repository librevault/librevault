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
#include "config/FolderSettings.h"
#include "util/BandwidthCounter.h"
#include "util/Endpoint.h"
#include <QHostAddress>
#include <QObject>
#include <QSet>
#include <QWebSocket>

namespace librevault {

class Peer;
class FolderGroup;
class NodeKey;
class BTProvider;
class DHTProvider;
class MulticastProvider;
class DHTGroup;
class BTGroup;
class MulticastGroup;

class PeerPool : public QObject {
  Q_OBJECT

 public:
  PeerPool(const FolderSettings& params, NodeKey* node_key, BTProvider* bt, DHTProvider* dht,
      MulticastProvider* multicast, QObject* parent);
  ~PeerPool() override;

  Q_SIGNAL void newValidPeer(Peer* peer);

  Q_SLOT void handleDiscovered(const Endpoint& endpoint);
  Q_SLOT void handleNewUrl(const QUrl& url);
  Q_SLOT void handleIncoming(QWebSocket* socket);

  /* Getters */
  QList<Peer*> peers() const { return peers_.toList(); }
  QList<Peer*> validPeers() const { return peers_ready_.toList(); }

  BandwidthCounter* getBlockCounterAll() { return &bc_all_; }
  BandwidthCounter* getBlockCounterBlocks() { return &bc_blocks_; }

  inline const FolderSettings& params() const { return params_; }

 private:
  FolderSettings params_;
  NodeKey* node_key_;
  BandwidthCounter bc_all_, bc_blocks_;

  DHTGroup* dht_group_;
  BTGroup* bt_group_;
  MulticastGroup* multicast_group_;

  /* Members */
  QSet<Peer*> peers_;
  QSet<Peer*> peers_ready_;

  // Member lookup optimization
  QSet<QByteArray> digests_;
  EndpointSet endpoints_;

  Q_SLOT void handleHandshake(Peer* peer);
  Q_SLOT void handleDisconnected(Peer* peer);
  Q_SLOT void handleConnected(Peer* peer);

  bool contains(Peer* peer) const;
};

} /* namespace librevault */
