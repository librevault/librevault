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
#include "MulticastGroup.h"
#include "control/Config.h"
#include "discovery/DiscoveryGroup.h"
#include <MulticastDiscovery.pb.h>

namespace librevault {

MulticastGroup::MulticastGroup(QUdpSocket* socket4, QUdpSocket* socket6, DiscoveryGroup* parent) : QObject(parent),
	socket4_(socket4), socket6_(socket6), folderid_(parent->folderid()) {
	timer_ = new QTimer(this);
	timer_->setInterval(Config::get()->global_get("multicast_repeat_interval").asUInt64()*1000);
	connect(timer_, &QTimer::timeout, this, &MulticastGroup::sendMulticasts);

	timer_->start();
}

MulticastGroup::~MulticastGroup() {}

void MulticastGroup::useDiscoveredResult(QByteArray folderid, DiscoveryResult result) {
	if(folderid == folderid_)
		emit discovered(result);
}

QByteArray MulticastGroup::get_message() {
	protocol::MulticastDiscovery message;
	message.set_port(Config::get()->global_get("p2p_listen").asUInt());
	message.set_folderid(folderid_.data(), folderid_.size());
	//message.set_pubkey(node_key_.public_key().data(), node_key_.public_key().size());

	QByteArray message_s(message.ByteSize(), 0);
	message.SerializeToArray(message_s.data(), message_s.size());
	return message_s;
}

void MulticastGroup::sendMulticast(QUdpSocket* socket, QHostAddress addr, quint16 port) {
	if(socket->writeDatagram(get_message(), addr, port))
		qDebug() << "===> Multicast message sent to: " << addr << ":" << port;
	else
		qDebug() << "=X=> Multicast message not sent to: " << addr << ":" << port;
}

void MulticastGroup::sendMulticasts() {
	sendMulticast(socket4_, QHostAddress("239.192.152.144"), 28914);
	sendMulticast(socket6_, QHostAddress("ff08::BD02"), 28914);
}

} /* namespace librevault */
