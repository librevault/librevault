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
#include "RemoteFolder.h"

namespace librevault {

RemoteFolder::RemoteFolder(QObject* parent) : QObject(parent) {}
RemoteFolder::~RemoteFolder() {}

QString RemoteFolder::log_tag() const {
	return "[" + displayName() + "] ";
}

/* InterestGuard */
RemoteFolder::InterestGuard::InterestGuard(RemoteFolder* remote) : remote_(remote) {
	remote_->interest();
}

RemoteFolder::InterestGuard::~InterestGuard() {
	remote_->uninterest();
}

std::shared_ptr<RemoteFolder::InterestGuard> RemoteFolder::get_interest_guard() {
	try {
		return std::shared_ptr<InterestGuard>(interest_guard_);
	}catch(std::bad_weak_ptr& e){
		auto guard = std::make_shared<InterestGuard>(this);
		interest_guard_ = guard;
		return guard;
	}
}

} /* namespace librevault */
