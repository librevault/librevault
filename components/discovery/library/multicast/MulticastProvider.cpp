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
#include "MulticastProvider.h"
#include "MulticastGroup.h"
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>

Q_LOGGING_CATEGORY(log_multicast, "discovery.multicast")

namespace librevault {

MulticastProvider::MulticastProvider(QObject* parent) : QObject(parent) {
	socket4_ = new QUdpSocket(this);
	socket6_ = new QUdpSocket(this);

	connect(socket4_, &QUdpSocket::readyRead, [=]{processDatagram(socket4_);});
	connect(socket6_, &QUdpSocket::readyRead, [=]{processDatagram(socket6_);});
}

MulticastProvider::~MulticastProvider() {
	stop();
}

void MulticastProvider::start(QHostAddress addr4, quint16 port4, QHostAddress addr6, quint16 port6) {
	addr4_ = addr4;
	multicast_port4_ = port4;
	addr6_ = addr6;
	multicast_port6_ = port6;

	if(! socket4_->bind(QHostAddress::AnyIPv4, multicast_port4_))
		qCWarning(log_multicast) << "Could not bind MulticastProvider's IPv4 socket:" << socket4_->errorString();
	if(! socket6_->bind(QHostAddress::AnyIPv6, multicast_port6_))
		qCWarning(log_multicast) << "Could not bind MulticastProvider's IPv6 socket:" << socket6_->errorString();

	if(! socket4_->joinMulticastGroup(addr4_))
		qCWarning(log_multicast) << "Could not join IPv4 multicast group:" << socket4_->errorString();
	if(! socket6_->joinMulticastGroup(addr6_))
		qCWarning(log_multicast) << "Could not join IPv6 multicast group:" << socket6_->errorString();

	socket4_->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);
	socket6_->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);
}

void MulticastProvider::stop() {
	socket4_->leaveMulticastGroup(addr4_);
	socket6_->leaveMulticastGroup(addr6_);

	socket4_->close();
	socket6_->close();
}

void MulticastProvider::processDatagram(QUdpSocket* socket) {
	char datagram_buffer[buffer_size_];
	QHostAddress peer_address;
	quint16 peer_port;
	qint64 datagram_size = socket->readDatagram(datagram_buffer, buffer_size_, &peer_address, &peer_port);

	// JSON parsing
	QJsonObject message = QJsonDocument::fromJson(QByteArray(datagram_buffer, datagram_size)).object();
	if(! message.isEmpty()) {
		qCDebug(log_multicast) << "<=== Multicast message received from:" << peer_address << ":" << peer_port;

		QByteArray id = QByteArray::fromBase64(message["id"].toString().toUtf8());
		quint16 announce_port = message["port"].toInt();

		emit discovered(id, peer_address, announce_port);
	}else{
		qCDebug(log_multicast) << "<=X= Malformed multicast message from:" << peer_address << ":" << peer_port;
	}
}

} /* namespace librevault */
