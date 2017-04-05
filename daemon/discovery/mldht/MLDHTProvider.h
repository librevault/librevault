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
#include "discovery/btcompat.h"
#include "discovery/DiscoveryResult.h"
#include <QLoggingCategory>
#include <QHostInfo>
#include <QTimer>
#include <QUdpSocket>

Q_DECLARE_LOGGING_CATEGORY(log_dht)

namespace librevault {

class PortMapper;
class StateCollector;
class MLDHTProvider : public QObject {
	Q_OBJECT
public:
	MLDHTProvider(PortMapper* port_mapping, StateCollector* state_collector, QObject* parent);
	virtual ~MLDHTProvider();

	void pass_callback(void* closure, int event, const uint8_t* info_hash, const uint8_t* data, size_t data_len);

	int node_count() const;

	quint16 getPort();
	quint16 getExternalPort();

signals:
	void eventReceived(int event, btcompat::info_hash ih, QByteArray values);
	void discovered(QByteArray folderid, DiscoveryResult result);

public slots:
	void addNode(QHostAddress addr, quint16 port);

private:
	PortMapper* port_mapping_;
	StateCollector* state_collector_;

	using dht_id = btcompat::info_hash;
	dht_id own_id;

	// Sockets
	QUdpSocket* socket_;
	QTimer* periodic_;

	// Initialization
	void init();
	void readSessionFile();

	void deinit();
	void writeSessionFile();

	static constexpr size_t buffer_size_ = 65535;
	void processDatagram();

	void periodic_request();

	QMap<int, quint16> resolves_;

private slots:
	void handle_resolve(const QHostInfo& host);
};

} /* namespace librevault */
