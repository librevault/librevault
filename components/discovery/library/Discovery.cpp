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
#include "DiscoveryGroup.h"
#include "multicast/MulticastGroup.h"
#include "multicast/MulticastProvider.h"
#include "mldht/MLDHTGroup.h"
#include "mldht/MLDHTProvider.h"
//#include "bttracker/BTTrackerGroup.h"
//#include "bttracker/BTTrackerProvider.h"

namespace librevault {

Discovery::Discovery(QObject* parent) : QObject(parent) {
	multicast_ = new MulticastProvider(this);
	//bttracker_ = new BTTrackerProvider(portmapper, this);
	mldht_ = new MLDHTProvider(this);
	connect(mldht_, &MLDHTProvider::nodeCountChanged, this, &Discovery::DHTnodeCountChanged);
}

Discovery::~Discovery() {}

DiscoveryGroup* Discovery::createGroup(QByteArray id) {
	return new DiscoveryGroup(id, multicast_, mldht_, this);
}

void Discovery::setAnnounceLANPort(quint16 port) {
	multicast_->setAnnouncePort(port);
}

void Discovery::setAnnounceWANPort(quint16 port) {
	mldht_->setAnnouncePort(port);
}

// Multicast
void Discovery::startMulticast(QHostAddress addr4, quint16 port4, QHostAddress addr6, quint16 port6) {
	multicast_->start(addr4, port4, addr6, port6);
}

void Discovery::stopMulticast() {
	multicast_->stop();
}

// DHT
void Discovery::startDHT(quint16 port) {
	mldht_->start(port);
}

void Discovery::stopDHT() {
	mldht_->stop();
}

void Discovery::addDHTRouter(QString host, quint16 port) {
	mldht_->addRouter(host, port);
}

} /* namespace librevault */
