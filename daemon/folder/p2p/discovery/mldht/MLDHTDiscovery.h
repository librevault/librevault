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
#include "pch.h"
#include "../DiscoveryService.h"
#include "../btcompat.h"
#include <boost/bimap.hpp>

namespace librevault {

class MLDHTSearcher;

class MLDHTDiscovery : public DiscoveryService {
public:
	MLDHTDiscovery(Client& client, P2PProvider& provider);
	virtual ~MLDHTDiscovery();

	void pass_callback(void* closure, int event, const uint8_t* info_hash, const uint8_t* data, size_t data_len);

	uint_least32_t node_count() const;

	bool active_v4() const {return socket4_.is_open();}
	bool active_v6() const {return socket6_.is_open();}

	void register_group(std::shared_ptr<FolderGroup> group_ptr);
	void unregister_group(std::shared_ptr<FolderGroup> group_ptr);
protected:
	P2PProvider& provider_;

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
	void init();
	bool dht_initialized = false;

	void deinit();
	void deinit_session_file();

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

static void lv_dht_callback_glue(void* closure, int event, const unsigned char* info_hash, const void* data, size_t data_len) {
	((librevault::MLDHTDiscovery*)closure)->pass_callback(closure, event, info_hash, (const uint8_t*)data, data_len);
}

} /* extern "C" */
