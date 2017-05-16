/* Copyright (C) 2017 Alexander Shishenko <alex@shishenko.com>
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
#include "DiscoveryAdapter.h"
#include "control/Config.h"
#include <DiscoveryGroup.h>

namespace librevault {

DiscoveryAdapter::DiscoveryAdapter(PortMapper* portmapper, QObject* parent) : QObject(parent), portmapper_(portmapper) {
	discovery_ = new Discovery(this);

	initDiscovery();

	quint16 lan_port = Config::get()->getGlobal("p2p_listen").toUInt();
	quint16 wan_port = portmapper_->getMappedPort("main");
	connect(portmapper_, &PortMapper::portMapped, this, [=](QString id, quint16 port){
		if(id == "main") discovery_->setAnnounceWANPort(port);
	});
	discovery_->setAnnounceLANPort(lan_port);
	discovery_->setAnnounceWANPort(wan_port);
}

DiscoveryAdapter::~DiscoveryAdapter() {
	portmapper_->removePort("mldht");
}

DiscoveryGroup* DiscoveryAdapter::createGroup(QByteArray folderid) {
	DiscoveryGroup* dgroup = discovery_->createGroup(folderid);

	bool dht_enabled = Config::get()->getFolder(folderid).value("mainline_dht_enabled").toBool();
	bool bt_enabled = Config::get()->getFolder(folderid).value("bt_enabled").toBool();
	bool multicast_enabled = Config::get()->getFolder(folderid).value("multicast_enabled").toBool();
	dgroup->setDHTEnabled(dht_enabled);
	dgroup->setBTEnabled(bt_enabled);
	dgroup->setMulticastEnabled(multicast_enabled);
	dgroup->setMulticastInterval(std::chrono::seconds(30));

	if(bt_enabled) {
		QStringList bt_trackers = Config::get()->getGlobal("bt_trackers").toStringList();
		QList<QUrl> trackers;
		for(const QString& tracker : qAsConst(bt_trackers))
			trackers.push_back(QUrl(tracker));

		dgroup->setBTTrackers(trackers);
	}

	return dgroup;
}

void DiscoveryAdapter::initDiscovery() {
	bool dht_enabled = Config::get()->getGlobal("mainline_dht_enbled").toBool();
	quint16 dht_port = Config::get()->getGlobal("mainline_dht_port").toUInt();
	portmapper_->addPort("mldht", dht_port, QAbstractSocket::UdpSocket, "Librevault DHT");

	if(dht_enabled) {
		discovery_->startDHT(dht_port);
		discovery_->addDHTRouter("router.utorrent.com", 6881);
		discovery_->addDHTRouter("router.bittorrent.com", 6881);
		discovery_->addDHTRouter("dht.transmissionbt.com", 6881);
		discovery_->addDHTRouter("dht.aelitis.com", 6881);
		discovery_->addDHTRouter("dht.libtorrent.org", 25401);
	}

	bool multicast_enabled = Config::get()->getGlobal("multicast_enabled").toBool();
	if(multicast_enabled) {
		discovery_->startMulticast(QHostAddress("239.192.152.144"), 28914, QHostAddress("ff08::BD02"), 28914);
	}
}

} /* namespace librevault */
