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
#include "MLDHTSearcher.h"
#include "MLDHTDiscovery.h"
#include "Client.h"
#include "folder/FolderGroup.h"
#include "folder/p2p/P2PProvider.h"
#include "folder/p2p/P2PFolder.h"
#include "folder/p2p/WSServer.h"
#include "nat/PortMappingService.h"
#include <dht.h>
#include <librevault/crypto/Hex.h>

namespace librevault {

using namespace boost::asio::ip;

MLDHTSearcher::MLDHTSearcher(std::weak_ptr<FolderGroup> group, MLDHTDiscovery& service, PortMappingService& port_mapping) :
		DiscoveryInstance(group, service),
		port_mapping_(port_mapping),
		search_timer6_(service.client().network_ios()),
		search_timer4_(service.client().network_ios()) {
	info_hash_ = btcompat::get_info_hash(std::shared_ptr<FolderGroup>(group)->hash());

	set_enabled(std::shared_ptr<FolderGroup>(group)->params().mainline_dht_enabled);

	attached_connection_ = std::shared_ptr<FolderGroup>(group)->attached_signal.connect([&, this](std::shared_ptr<P2PFolder> p2p_folder){
		if(enabled_) {
			dht_ping_node(p2p_folder->remote_endpoint().data(), p2p_folder->remote_endpoint().size());
			LOGD("Added a new DHT node: " << p2p_folder->remote_endpoint());
		}
	});
}

void MLDHTSearcher::set_enabled(bool enable) {
	if(enable && !enabled_) {
		enabled_ = true;
		service_.client().network_ios().post([&, this]() {
			start_search(AF_INET);
			start_search(AF_INET6);
		});
	}else if(!enable && enabled_) {
		enabled_ = false;

		search_timer6_.cancel();
		search_timer4_.cancel();
	}
}

void MLDHTSearcher::start_search(int af) {
	bool announce = true;

	if(enabled_) {
		bool ready6 = (af == AF_INET6 && ((MLDHTDiscovery&)service_).active_v6());
		bool ready4 = (af == AF_INET && ((MLDHTDiscovery&)service_).active_v4());

		if(ready6 || ready4) {
			uint16_t public_port = port_mapping_.get_port_mapping("main");

			LOGD("Starting "
				<< (ready6 ? "IPv6" : "IPv4") << " "
				<< (announce ? "announce" : "search")
				<< " for: " << crypto::Hex().to_string(info_hash_)
				<< (announce ? " on port: " : "") << (announce ? std::to_string(public_port) : std::string()));

			dht_search(info_hash_.data(), announce ? public_port : 0, af, lv_dht_callback_glue, (MLDHTDiscovery*)&service_);
		}
	}

	boost::asio::steady_timer* timer;

	if(af == AF_INET6)
		timer = &search_timer6_;
	else
		timer = &search_timer4_;

	timer->expires_from_now(std::chrono::minutes(1));

	timer->async_wait(std::bind([this](int af, bool announce, const boost::system::error_code& error){
		if(error == boost::asio::error::operation_aborted) return;
		start_search(af);
	}, af, announce, std::placeholders::_1));
}

void MLDHTSearcher::search_completed(bool start_v4, bool start_v6) {

}

} /* namespace librevault */
