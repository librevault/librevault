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
#include <QTimer>
#include <QUdpSocket>
#include <chrono>

namespace librevault {

class MulticastProvider;
class MulticastGroup : public QObject {
	Q_OBJECT

signals:
	void discovered(QHostAddress addr, quint16 port);

public:
	MulticastGroup(MulticastProvider* provider, QByteArray id);
	void setEnabled(bool enabled);
	void setInterval(std::chrono::seconds interval);

private:
	MulticastProvider* provider_;
	QTimer* timer_;
	QByteArray id_;

	QByteArray getMessage();
	void sendMulticast(QUdpSocket* socket, QHostAddress addr, quint16 port);

private slots:
	void sendMulticasts();
};

} /* namespace librevault */
