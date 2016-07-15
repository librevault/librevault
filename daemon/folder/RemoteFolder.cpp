/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
 */
#include "../Client.h"
#include "RemoteFolder.h"

namespace librevault {

RemoteFolder::RemoteFolder(Client& client) :
	AbstractFolder(client), client_(client) {}
RemoteFolder::~RemoteFolder() {}

/* InterestGuard */
RemoteFolder::InterestGuard::InterestGuard(std::shared_ptr<RemoteFolder> remote) : remote_(remote) {
	remote->interest();
}

RemoteFolder::InterestGuard::~InterestGuard() {
	auto folder_ptr = remote_.lock();
	if(folder_ptr)
		folder_ptr->uninterest();
}

std::shared_ptr<RemoteFolder::InterestGuard> RemoteFolder::get_interest_guard() {
	try {
		return std::shared_ptr<InterestGuard>(interest_guard_);
	}catch(std::bad_weak_ptr& e){
		auto guard = std::make_shared<InterestGuard>(shared_from_this());
		interest_guard_ = guard;
		return guard;
	}
}

} /* namespace librevault */
