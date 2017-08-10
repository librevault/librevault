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
#include "BTGroup.h"
#include "BTConnection.h"

namespace librevault {

BTGroup::BTGroup(BTProvider* provider, QByteArray discovery_id, QObject* parent) :
	QObject(parent),
	provider_(provider),
	discovery_id_(discovery_id) {}

void BTGroup::setEnabled(bool enabled) {
	enabled_ = enabled;
	for(auto& i : connections_)
		i.second->setEnabled(enabled_);
}

void BTGroup::setTrackerList(QList<QUrl> trackers) {
	connections_.clear();
	for(QUrl tracker : qAsConst(trackers)) {
		connections_[tracker] = std::make_unique<BTConnection>(tracker, this, provider_);
		connections_[tracker]->setEnabled(enabled_);
	}
}

} /* namespace librevault */
