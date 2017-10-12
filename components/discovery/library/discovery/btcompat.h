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
#include <QByteArray>
#include <QHostAddress>
#include <QtEndian>

namespace librevault {
namespace btcompat {

// Function declaration
inline QPair<QHostAddress, quint16> unpackEndpoint4(QByteArray packed) {
	packed = packed.leftJustified(6, 0, true);

	QHostAddress addr = QHostAddress(qFromBigEndian(*reinterpret_cast<quint32*>(packed.mid(0, 4).data())));
	quint16 port = qFromBigEndian(*reinterpret_cast<quint16*>(packed.mid(4, 2).data()));

	return {addr, port};
};

inline QPair<QHostAddress, quint16> unpackEndpoint6(QByteArray packed) {
	packed = packed.leftJustified(18, 0, true);

	QHostAddress addr = QHostAddress(reinterpret_cast<quint8*>(packed.mid(0, 16).data()));
	quint16 port = qFromBigEndian(*reinterpret_cast<quint16*>(packed.mid(16, 2).data()));

	return {addr, port};
};

inline QList<QPair<QHostAddress, quint16>> unpackEnpointList4(const QByteArray& packed) {
	QList<QPair<QHostAddress, quint16>> l;
	for(int i=0; i < packed.size(); i += 6) {
		l.push_back(unpackEndpoint4(packed.mid(i, 6)));
	}
	return l;
};

inline QList<QPair<QHostAddress, quint16>> unpackEnpointList6(const QByteArray& packed) {
	QList<QPair<QHostAddress, quint16>> l;
	for(int i=0; i < packed.size(); i += 18) {
		l.push_back(unpackEndpoint6(packed.mid(i, 18)));
	}
	return l;
};

} /* namespace btcompat */
} /* namespace librevault */
