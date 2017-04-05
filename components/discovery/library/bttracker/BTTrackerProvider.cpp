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
#include "BTTrackerProvider.h"
#include "Discovery.h"
#include "rand.h"
#include <QtEndian>

namespace librevault {

BTTrackerProvider::BTTrackerProvider(PortMapper* portmapping, Discovery* parent) : QObject(parent),
	portmapping_(portmapping), parent_(parent) {
	socket_ = new QUdpSocket();
	socket_->bind();

	connect(socket_, &QUdpSocket::readyRead, this, &BTTrackerProvider::processDatagram);

	// Generate new peer id
	peer_id_ = peer_id_prefix_ + getRandomArray(20);
	peer_id_.resize(20);
}

BTTrackerProvider::~BTTrackerProvider() {}

quint16 BTTrackerProvider::getAnnounceWANPort() const {
	return parent_->getAnnounceWANPort();
}

btcompat::peer_id BTTrackerProvider::getPeerId() const {
	return btcompat::get_peer_id(peer_id_);
}

void BTTrackerProvider::processDatagram() {
	char datagram_buffer[buffer_size_];
	qint64 datagram_size = socket_->readDatagram(datagram_buffer, buffer_size_);

	QByteArray message(datagram_buffer, datagram_size);
	if(message.size() >= 8) {
		quint32 action, transaction_id;
		std::copy(message.data()+0, message.data()+4, reinterpret_cast<char*>(&action));
		std::copy(message.data()+4, message.data()+8, reinterpret_cast<char*>(&transaction_id));

		emit receivedMessage(qFromBigEndian(action), transaction_id, message);
	}
}

} /* namespace librevault */
