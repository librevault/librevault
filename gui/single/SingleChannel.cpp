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
#include "SingleChannel.h"
#include <QCoreApplication>

SingleChannel::SingleChannel(QString arg) :
		QUdpSocket() {
	bool bound = bind(QHostAddress::LocalHost, 42343);
	if(bound) {
		connect(this, &SingleChannel::readyRead, this, &SingleChannel::datagramReceived);
	}else{

		if(arg.isEmpty())
			writeDatagram("show", QHostAddress::LocalHost, 42343);
		else
			writeDatagram(QString("arg ").toLatin1()+arg.toUtf8(), QHostAddress::LocalHost, 42343);
		exit(1);
	}
}

SingleChannel::~SingleChannel() {}

void SingleChannel::datagramReceived() {
	while(hasPendingDatagrams()) {
		QByteArray datagram(pendingDatagramSize(), 0);
		QHostAddress sender;
		uint16_t port;

		readDatagram(datagram.data(), datagram.size(), &sender, &port);

		if(datagram == "show")
			emit showMainWindow();
		if(datagram.startsWith("arg "))
			emit openLink(QString::fromUtf8(datagram).mid(4));
	}
}
