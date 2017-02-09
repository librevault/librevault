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
#include "util/log.h"
#include <QHostInfo>
#include <QTimer>
#include <QUdpSocket>
#include <QUrl>

namespace librevault {

class BTTrackerProvider;
class BTTrackerGroup;
class FolderGroup;

// BEP-0015 partial implementation (without scrape mechanism)
class BTTrackerConnection : public QObject {
	Q_OBJECT
	LOG_SCOPE("BTTrackerConnection");
public:
	BTTrackerConnection(QUrl tracker_address, BTTrackerGroup* btgroup_, BTTrackerProvider* tracker_provider);
	virtual ~BTTrackerConnection();

	void setEnabled(bool enabled);

signals:
	void discovered(DiscoveryResult result);

private:
	BTTrackerProvider* provider_;
	BTTrackerGroup* btgroup_;

	QUrl tracker_address_;

	unsigned int announced_times_ = 0;

	QUdpSocket* socket_;

	// Tracker address
	QHostAddress addr_;
	quint16 port_;

	// Connection state
	quint64 connection_id_ = 0;
	quint32 transaction_id_connect_ = 0;
	quint32 transaction_id_announce4_ = 0;
	quint32 transaction_id_announce6_ = 0;

	// Timers
	QTimer* resolver_timer_;
	QTimer* connect_timer_;
	QTimer* announce_timer_;
	int resolver_lookup_id_ = 0;

	quint32 gen_transaction_id();

	void resolve();
	void btconnect();
	void announce();

private slots:
	void handle_message(quint32 action, quint32 transaction_id, QByteArray message);

	void handle_resolve(const QHostInfo& host);
	void handle_connect(QByteArray message);
	void handle_announce(QByteArray message);
};

} /* namespace librevault */
