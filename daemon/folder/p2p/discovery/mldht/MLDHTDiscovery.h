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
#include "../DiscoveryService.h"
#include "../btcompat.h"
#include <boost/bimap.hpp>

namespace librevault {

class MLDHTSearcher;

class MLDHTDiscovery : public DiscoveryService {
public:
	MLDHTDiscovery(Client& client);
	virtual ~MLDHTDiscovery();

	void pass_callback(void* closure, int event, const uint8_t* info_hash, const uint8_t* data, size_t data_len);

	bool active_v4() {return socket4_.is_open();}
	bool active_v6() {return socket6_.is_open();}

	void register_group(std::shared_ptr<FolderGroup> group_ptr);
	void unregister_group(std::shared_ptr<FolderGroup> group_ptr);
protected:

	std::map<btcompat::info_hash, std::shared_ptr<FolderGroup>> groups_;
	std::map<btcompat::info_hash, std::unique_ptr<MLDHTSearcher>> searchers_;
	//std::chrono::seconds repeat_interval_ = std::chrono::seconds(0);

	using dht_id = btcompat::info_hash;
	dht_id own_id;

	// Sockets
	udp_socket socket4_;
	udp_socket socket6_;
	udp_resolver resolver_;

	// Initialization
	void deinit();
	void init();
	void init_id();
	bool initialized = false;


	using udp_buffer = std::array<char, 65536>;
	void process(udp_socket* socket, std::shared_ptr<udp_buffer> buffer, size_t size, std::shared_ptr<udp_endpoint> endpoint_ptr, const boost::system::error_code& ec);
	void receive(udp_socket& socket);

	std::mutex dht_mutex;

	time_t tosleep = 0;
	boost::asio::steady_timer tosleep_timer_;
	void maintain_periodic_requests();
};

} /* namespace librevault */

extern "C" {

struct lv_dht_closure {
	librevault::MLDHTDiscovery* discovery_ptr;
};

static void lv_dht_callback_glue(void* closure, int event, const unsigned char* info_hash, const void* data, size_t data_len) {
	((lv_dht_closure*)closure)->discovery_ptr->pass_callback(closure, event, info_hash, (const uint8_t*)data, data_len);
}

} /* extern "C" */
