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
#include "btcompat.h"
#include <QByteArray>
#include <QTimer>
#include <QUdpSocket>

namespace librevault {

class DHTWrapper : public QObject {
	Q_OBJECT

public:
	using EndpointList = QList<QPair<QHostAddress, quint16>>;

signals:
	void searchDone(QByteArray id, QAbstractSocket::NetworkLayerProtocol af, EndpointList nodes);
	void foundNodes(QByteArray id, QAbstractSocket::NetworkLayerProtocol af, EndpointList nodes);
	void nodeCountChanged(int node_count);

public:
	DHTWrapper(QUdpSocket* socket4, QUdpSocket* socket6, QByteArray own_id, QObject* parent);
	~DHTWrapper();

	void nodeCount(int& good_return, int& dubious_return, int& cached_return, int& incoming_return);
	int goodNodeCount();

	EndpointList getNodes();

public slots:
	void pingNode(QHostAddress addr, quint16 port);
	void startAnnounce(QByteArray id, QAbstractSocket::NetworkLayerProtocol af, quint16 port);
	void startSearch(QByteArray id, QAbstractSocket::NetworkLayerProtocol af);

	void enable() {periodic_->start();}
	void disable() {periodic_->stop();}

private:
	QTimer* periodic_;
	int last_node_count_ = 0;

	int convertAF(QAbstractSocket::NetworkLayerProtocol qaf);
	bool enabled() {return periodic_->isActive();}

private slots:
	void processDatagram(QUdpSocket* socket);
	void periodicRequest();
	void updateNodeCount();
};

} /* namespace librevault */
