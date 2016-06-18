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
#pragma once
#include "pch.h"
#include "folder/p2p/discovery/DiscoveryInstance.h"
#include "../btcompat.h"

namespace librevault {

class MLDHTDiscovery;

class MLDHTSearcher : public DiscoveryInstance, public Loggable {
public:
	MLDHTSearcher(std::weak_ptr<FolderGroup> group, MLDHTDiscovery& service);

	void set_enabled(bool enable);
	void start_search(bool start_v4, bool start_v6);

private:
	btcompat::info_hash info_hash_;
	boost::signals2::scoped_connection attached_connection_;

	bool enabled_ = false;
};

} /* namespace librevault */
