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
#include <QLoggingCategory>
#include <QHostInfo>
#include <QTimer>
#include <QUdpSocket>

Q_DECLARE_LOGGING_CATEGORY(log_dht)

namespace librevault {

class Discovery;
class DHTWrapper;
class MLDHTProvider : public QObject {
	Q_OBJECT

signals:
	void discovered(QByteArray ih, QHostAddress addr, quint16 port);
	void nodeCountChanged(int count);

public:
	MLDHTProvider(Discovery* parent);
	virtual ~MLDHTProvider();

	// Start/Stop
	void start(quint16 port);
	void stop();

	int getNodeCount() const;
	quint16 getAnnouncePort() const {return announce_port_;}

	QList<QPair<QHostAddress, quint16>> getNodes();

public slots:
	void addRouter(QString host, quint16 port);
	void addNode(QHostAddress addr, quint16 port);
	void setAnnouncePort(quint16 port) {announce_port_ = port;}

	// internal
	void startAnnounce(QByteArray id, QAbstractSocket::NetworkLayerProtocol af, quint16 port);
	void startSearch(QByteArray id, QAbstractSocket::NetworkLayerProtocol af);

private:
	Discovery* parent_;
	DHTWrapper* dht_wrapper_ = nullptr;

	quint16 announce_port_;

	// Sockets
	QUdpSocket* socket4_;
	QUdpSocket* socket6_;

	// Initialization
	void readSessionFile(QString path);
	void writeSessionFile(QString path);

	QMap<int, quint16> resolves_;

private slots:
	void handleResolve(const QHostInfo& host);
	void handleSearch(QByteArray id, QAbstractSocket::NetworkLayerProtocol af, QList<QPair<QHostAddress, quint16>> nodes);
};

} /* namespace librevault */
