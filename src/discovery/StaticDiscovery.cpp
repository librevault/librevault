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
#include "StaticDiscovery.h"
#include "../Client.h"
#include "../directory/ExchangeGroup.h"
#include "../directory/p2p/P2PProvider.h"
#include "../directory/fs/FSDirectory.h"

namespace librevault {

using namespace boost::asio::ip;

StaticDiscovery::StaticDiscovery(Client& client) :
	DiscoveryService(client) {}

StaticDiscovery::~StaticDiscovery() {}

void StaticDiscovery::register_group(std::shared_ptr<ExchangeGroup> group_ptr) {
	for(auto& node : group_ptr->fs_dir()->folder_config().nodes){
		add_node(node, group_ptr);
	}
}

void StaticDiscovery::unregister_group(std::shared_ptr<ExchangeGroup> group_ptr) {
	groups_.erase(group_ptr);
}

} /* namespace librevault */