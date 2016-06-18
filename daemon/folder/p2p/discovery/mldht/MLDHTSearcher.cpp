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
#include "MLDHTSearcher.h"
#include "MLDHTDiscovery.h"
#include "Client.h"
#include "folder/FolderGroup.h"
#include "folder/p2p/P2PProvider.h"
#include "folder/p2p/P2PFolder.h"
#include "folder/p2p/WSServer.h"
#include <dht.h>

namespace librevault {

using namespace boost::asio::ip;

MLDHTSearcher::MLDHTSearcher(std::weak_ptr<FolderGroup> group, MLDHTDiscovery& service) :
	DiscoveryInstance(group, service), Loggable("MLDHTSearcher") {
	info_hash_ = btcompat::get_info_hash(std::shared_ptr<FolderGroup>(group)->hash());

	set_enabled(std::shared_ptr<FolderGroup>(group)->params().mainline_dht_enabled);

	attached_connection_ = std::shared_ptr<FolderGroup>(group)->attached_signal.connect([&, this](std::shared_ptr<P2PFolder> p2p_folder){
		if(enabled_) {
			dht_ping_node(p2p_folder->remote_endpoint().data(), p2p_folder->remote_endpoint().size());
			log_->debug() << log_tag() << "Added a new DHT node: " << p2p_folder->remote_endpoint();
		}
	});
}

void MLDHTSearcher::set_enabled(bool enable) {
	if(enable && !enabled_) {
		enabled_ = true;
		start_search(true, true);
	}
}

void MLDHTSearcher::start_search(bool start_v4, bool start_v6) {
	if(enabled_ && start_v4 && ((MLDHTDiscovery&)service_).active_v4()) {
		log_->debug() << log_tag() << "Starting search for: " << crypto::Hex().to_string(info_hash_);
		lv_dht_closure* closure = new lv_dht_closure();
		closure->discovery_ptr = (MLDHTDiscovery*)&service_;

		dht_search(info_hash_.data(), service_.client().p2p_provider()->public_port(), AF_INET, lv_dht_callback_glue, closure);
	}
	if(enabled_ && start_v6 && ((MLDHTDiscovery&)service_).active_v6()) {
		lv_dht_closure* closure = new lv_dht_closure();
		closure->discovery_ptr = (MLDHTDiscovery*)&service_;

		dht_search(info_hash_.data(), service_.client().p2p_provider()->public_port(), AF_INET6, lv_dht_callback_glue, closure);
	}
}

} /* namespace librevault */
