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
#include <QTimer>

namespace librevault {

class MLDHTProvider;
class FolderGroup;

class MLDHTGroup : public QObject {
	Q_OBJECT

signals:
	void discovered(QHostAddress addr, quint16 port);

public:
	MLDHTGroup(MLDHTProvider* provider, QByteArray id);
	bool enabled() {return timer_->isActive();}

public slots:
	void setEnabled(bool enable);

private:
	MLDHTProvider* provider_;
	QTimer* timer_;

	QByteArray id_;

	inline QByteArray getInfoHash() {return id_.leftJustified(20, 0, true);}

private slots:
	void startSearches();
	void handleDiscovered(QByteArray ih, QHostAddress addr, quint16 port);
};

} /* namespace librevault */
