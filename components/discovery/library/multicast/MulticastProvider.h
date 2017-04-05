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
#include <QUdpSocket>

namespace librevault {

class FolderGroup;
class MulticastGroup;
class MulticastProvider : public QObject {
	Q_OBJECT

signals:
	void discovered(QByteArray id, QHostAddress addr, quint16 port);

public:
	explicit MulticastProvider(QObject* parent);
	virtual ~MulticastProvider();

	quint16 getAnnouncePort() const {return announce_port_;}
	quint16 getMulticastPort4() const {return multicast_port4_;}
	quint16 getMulticastPort6() const {return multicast_port4_;}

	QHostAddress getAddress4() const {return addr4_;}
	QHostAddress getAddress6() const {return addr6_;}

	QUdpSocket* getSocket4() {return socket4_;}
	QUdpSocket* getSocket6() {return socket6_;}

public slots:
	void start(QHostAddress addr4, quint16 port4, QHostAddress addr6, quint16 port6);
	void stop();
	void setAnnouncePort(quint16 port) {announce_port_ = port;}

private:
	QHostAddress addr4_;
	QHostAddress addr6_;

	quint16 announce_port_, multicast_port4_, multicast_port6_;

	QUdpSocket* socket4_;
	QUdpSocket* socket6_;

	static constexpr size_t buffer_size_ = 65535;

private slots:
	void processDatagram(QUdpSocket* socket);
};

} /* namespace librevault */
