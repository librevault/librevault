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
#include "DiscoveryApp.h"
#include <docopt.h>

namespace librevault {

DiscoveryApp::DiscoveryApp(int argc, char** argv, const char* USAGE) : QCoreApplication(argc, argv) {
  std::map<std::string, docopt::value> args = docopt::docopt(USAGE, {argv + 1, argv + argc}, true);

  QByteArray discovery_id = QByteArray::fromHex(QByteArray::fromStdString(args["<id>"].asString()));

  // initialization
  mcast_p = new MulticastProvider(this);
  mcast_p->setAnnouncePort(12345);

  mcast_p->start(QHostAddress("239.192.152.144"), 28914);

  mcast_g = new MulticastGroup(mcast_p, discovery_id, this);
  mcast_g->setEnabled(true);

  connect(mcast_g, &MulticastGroup::discovered, this, &DiscoveryApp::handleDiscovered);

  //discovery->startMulticast(QHostAddress("239.192.152.144"), 28914, QHostAddress("ff08::BD02"), 28914);
  //discovery->startDHT(39893);
  //discovery->addDHTRouter("router.utorrent.com", 6881);
  //discovery->addDHTRouter("router.bittorrent.com", 6881);
  //discovery->addDHTRouter("dht.transmissionbt.com", 6881);
  //discovery->addDHTRouter("dht.aelitis.com", 6881);
  //discovery->addDHTRouter("dht.libtorrent.org", 25401);

  //connect(discovery, &Discovery::DHTnodeCountChanged, this, [=](int count){
  //	qDebug() << "DHT node count:" << count;
  //	qDebug() << discovery->getDHTNodes();
  //});

  // Group

//	dgroup = discovery->createGroup(id);
//	dgroup->setMulticastEnabled(true);
//	dgroup->setMulticastInterval(std::chrono::seconds(30));
//	dgroup->setDHTEnabled(true);
//	dgroup->setBTEnabled(true);
//	dgroup->setBTTrackers({
//		QUrl("udp://discovery.librevault.com:6969/announce"),
//	});
//
//	connect(dgroup, &DiscoveryGroup::discovered, this, &DiscoveryApp::handleDiscovered);
}

void DiscoveryApp::handleDiscovered(QHostAddress addr, quint16 port) {
  qInfo() << "Discovered:" << addr.toString() << port;
}

} /* namespace librevault */
