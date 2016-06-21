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
#include <util/file_util.h>
#include <MLDHTSessionFile.pb.h>
#include "MLDHTDiscovery.h"
#include "MLDHTSearcher.h"
#include "Client.h"
#include "folder/FolderGroup.h"
#include "folder/p2p/P2PProvider.h"
#include "folder/fs/FSFolder.h"
#include "dht.h"

namespace librevault {

using namespace boost::asio::ip;

MLDHTDiscovery::MLDHTDiscovery(Client& client) :
	DiscoveryService(client, "DHT"), socket4_(client_.network_ios()), socket6_(client_.network_ios()), resolver_(client_.network_ios()), tosleep_timer_(client_.network_ios()) {
	name_ = "MLDHTDiscovery";

	client.folder_added_signal.connect(std::bind(&MLDHTDiscovery::register_group, this, std::placeholders::_1));
	client.folder_removed_signal.connect(std::bind(&MLDHTDiscovery::unregister_group, this, std::placeholders::_1));

	init();
}

MLDHTDiscovery::~MLDHTDiscovery() {
	deinit();
}

void MLDHTDiscovery::init() {
	deinit();

	// We will restore our session from here
	MLDHTSessionFile session_pb;
	file_wrapper session_file(Config::get()->paths().dht_id_path, "r");
	if(!session_pb.ParseFromIstream(&session_file.ios())) throw;

	// Init id
	if(session_pb.id().size() == own_id.size()) {
		std::copy(session_pb.id().begin(), session_pb.id().end(), own_id.begin());
	}else{  // Invalid data
		CryptoPP::AutoSeededRandomPool rng;
		rng.GenerateBlock(own_id.data(), own_id.size());
	}

	// Init sockets
	try {
		bool v4_ready = false;
		bool v6_ready = false;
		try {
			socket4_.open(boost::asio::ip::udp::v4());
			socket4_.bind(udp_endpoint(address_v4::any(), (uint16_t)Config::get()->globals()["mainline_dht_port"].asUInt()));
			v4_ready = true;
		}catch(std::exception& e) {
			log_->warn() << log_tag() << "DHT IPv4 error: " << e.what();
		}

		try {
			socket6_.open(boost::asio::ip::udp::v6());
			socket6_.set_option(boost::asio::ip::v6_only(true));
			socket6_.bind(udp_endpoint(address_v6::any(), (uint16_t)Config::get()->globals()["mainline_dht_port"].asUInt()));
			v6_ready = true;
		}catch(std::exception& e) {
			log_->warn() << log_tag() << "DHT IPv6 error: " << e.what();
		}

		if(!v4_ready && !v6_ready) throw std::runtime_error("Both sockets are failed");

		int rc = dht_init(socket4_.native_handle(), socket6_.native_handle(), own_id.data(), nullptr);
		if(rc < 0) throw std::runtime_error("Internal DHT error");

		dht_initialized = true;
	}catch(std::exception& e){
		log_->warn() << log_tag() << "Could not initialize DHT: " << e.what();
		return;
	}

	// Init routers
	auto routers = Config::get()->globals()["mainline_dht_routers"];
	if(routers.isArray()) {
		for(auto& router_value : routers) {
			url router_url(router_value.asString());
			std::shared_ptr<udp_resolver::query> query = std::make_shared<udp_resolver::query>(router_url.host, std::to_string(router_url.port));
			resolver_.async_resolve(*query, std::bind([&, this](const boost::system::error_code& error, udp_resolver::iterator it, std::shared_ptr<udp_resolver::query>){
				for(; it != udp_resolver::iterator(); it++) {
					auto endpoint = it->endpoint();
					dht_ping_node(endpoint.data(), endpoint.size());
					log_->debug() << log_tag() << "Added a DHT router: " << it->host_name() << " Resolved: " << endpoint;
				}
			}, std::placeholders::_1, std::placeholders::_2, query));
		}
	}

	// Init nodes from file
	log_->notice() << "Loading " << session_pb.compact_endpoints6().size()+session_pb.compact_endpoints4().size() << " nodes from session file";
	if(active_v6()) {
		for(auto& compact_node6 : session_pb.compact_endpoints6()) {
			if(compact_node6.size() != sizeof(btcompat::compact_endpoint6)) continue;
			auto endpoint = btcompat::parse_compact_endpoint6(compact_node6.data());
			client_.network_ios().post([endpoint] {
				dht_ping_node(endpoint.data(), endpoint.size());
			});
		}
	}
	if(active_v4()) {
		for(auto& compact_node4 : session_pb.compact_endpoints4()) {
			if(compact_node4.size() != sizeof(btcompat::compact_endpoint4)) continue;
			auto endpoint = btcompat::parse_compact_endpoint4(compact_node4.data());
			client_.network_ios().post([endpoint] {
				dht_ping_node(endpoint.data(), endpoint.size());
			});
		}
	}

	// Start loops
	maintain_periodic_requests();

	if(active_v6())
		receive(socket6_);
	if(active_v4())
		receive(socket4_);
}

void MLDHTDiscovery::deinit() {
	tosleep_timer_.cancel();

	if(socket4_.is_open())
		socket4_.close();
	if(socket6_.is_open())
		socket6_.close();

	if(dht_initialized)
		deinit_session_file();

	if(dht_initialized)
		dht_uninit();
}

void MLDHTDiscovery::deinit_session_file() {
	MLDHTSessionFile session_pb;
	std::copy(own_id.begin(), own_id.end(), std::back_inserter(*session_pb.mutable_id()));

	struct sockaddr_in6 sa6[300];
	struct sockaddr_in sa4[300];

	int sa6_count = 300;
	int sa4_count = 300;

	dht_get_nodes(sa4, &sa4_count, sa6, &sa6_count);

	log_->notice() << log_tag() << "Saving " << sa4_count + sa6_count << " nodes to session file";

	for(auto i=0; i < sa6_count; i++) {
		btcompat::compact_endpoint6 endpoint6;
		std::copy((uint8_t*)&sa6[i].sin6_addr, (uint8_t*)&(sa6[i].sin6_addr)+16, endpoint6.ip6.data());
		endpoint6.port = boost::endian::big_to_native(sa6[i].sin6_port);
		std::string* endpoint_str = session_pb.add_compact_endpoints6();
		std::copy((const char*)&endpoint6, ((const char*)&endpoint6)+sizeof(btcompat::compact_endpoint6), std::back_inserter(*endpoint_str));
	}
	for(auto i=0; i < sa4_count; i++) {
		btcompat::compact_endpoint4 endpoint4;
		std::copy((uint8_t*)&sa4[i].sin_addr, (uint8_t*)&(sa4[i].sin_addr)+4, endpoint4.ip4.data());
		endpoint4.port = boost::endian::big_to_native(sa4[i].sin_port);
		std::string* endpoint_str = session_pb.add_compact_endpoints4();
		std::copy((const char*)&endpoint4, ((const char*)&endpoint4)+sizeof(btcompat::compact_endpoint4), std::back_inserter(*endpoint_str));
	}

	file_wrapper session_file(Config::get()->paths().dht_id_path, "w");
	session_pb.SerializeToOstream(&session_file.ios());
}

void MLDHTDiscovery::register_group(std::shared_ptr<FolderGroup> group_ptr) {
	groups_.insert({btcompat::get_info_hash(group_ptr->hash()), group_ptr});
	searchers_[btcompat::get_info_hash(group_ptr->hash())] = std::move(std::make_unique<MLDHTSearcher>(group_ptr, *this));
}

void MLDHTDiscovery::unregister_group(std::shared_ptr<FolderGroup> group_ptr) {
	searchers_.erase(btcompat::get_info_hash(group_ptr->hash()));
	groups_.erase(btcompat::get_info_hash(group_ptr->hash()));
}

uint_least32_t MLDHTDiscovery::node_count() const {
	int good6 = 0;
	int dubious6 = 0;
	int cached6 = 0;
	int incoming6 = 0;
	int good4 = 0;
	int dubious4 = 0;
	int cached4 = 0;
	int incoming4 = 0;

	if(active_v6())
		dht_nodes(AF_INET6, &good6, &dubious6, &cached6, &incoming6);
	if(active_v4())
		dht_nodes(AF_INET, &good4, &dubious4, &cached4, &incoming4);

	return good6+good4;
}

void MLDHTDiscovery::pass_callback(void* closure, int event, const uint8_t* info_hash, const uint8_t* data, size_t data_len) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION << " event: " << event;

	btcompat::info_hash ih; std::copy(info_hash, info_hash + ih.size(), ih.begin());

	auto folder_it = groups_.find(ih);
	if(folder_it == groups_.end()) return;

	std::shared_ptr<FolderGroup> folder_ptr = folder_it->second;

	if(event == DHT_EVENT_VALUES) {
		for(const uint8_t* data_cur = data; data_cur < data + data_len; data_cur += 6) {
			add_node(btcompat::parse_compact_endpoint4(data_cur), folder_ptr);
		}
	}else if(event == DHT_EVENT_VALUES6) {
		for(const uint8_t* data_cur = data; data_cur < data + data_len; data_cur += 18) {
			add_node(btcompat::parse_compact_endpoint6(data_cur), folder_ptr);
		}
	}else if(event == DHT_EVENT_SEARCH_DONE || event == DHT_EVENT_SEARCH_DONE6) {
		searchers_[btcompat::get_info_hash(folder_ptr->hash())]->search_completed(event == DHT_EVENT_SEARCH_DONE, event == DHT_EVENT_SEARCH_DONE6);
	}
}

void MLDHTDiscovery::process(udp_socket* socket, std::shared_ptr<udp_buffer> buffer, size_t size, std::shared_ptr<udp_endpoint> endpoint_ptr, const boost::system::error_code& ec) {
	if(ec == boost::asio::error::operation_aborted) return;

	log_->trace() << log_tag() << "DHT message received";

	std::unique_lock<std::mutex> lk(dht_mutex);
	dht_periodic(buffer.get()->data(), size, endpoint_ptr->data(), (int)endpoint_ptr->size(), &tosleep, lv_dht_callback_glue, this);
	lk.unlock();

	maintain_periodic_requests();

	receive(*socket);    // We received message, continue receiving others
}

void MLDHTDiscovery::receive(udp_socket& socket) {
	auto endpoint = std::make_shared<udp::endpoint>(socket.local_endpoint());
	auto buffer = std::make_shared<udp_buffer>();
	socket.async_receive_from(boost::asio::buffer(buffer->data(), buffer->size()), *endpoint,
		std::bind(&MLDHTDiscovery::process, this, &socket, buffer, std::placeholders::_2, endpoint, std::placeholders::_1));
}

void MLDHTDiscovery::maintain_periodic_requests() {
	tosleep_timer_.expires_from_now(std::chrono::seconds(tosleep));
	tosleep_timer_.async_wait([this](const boost::system::error_code& error){
		log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
		if(error == boost::asio::error::operation_aborted) return;

		std::unique_lock<std::mutex> lk(dht_mutex);
		dht_periodic(nullptr, 0, nullptr, 0, &tosleep, lv_dht_callback_glue, this);
		lk.unlock();

		maintain_periodic_requests();
	});
}

} /* namespace librevault */

// DHT library overrides
extern "C" {

int dht_blacklisted(const struct sockaddr *sa, int salen) {
	return 0;
}

void dht_hash(void *hash_return, int hash_size, const void *v1, int len1, const void *v2, int len2, const void *v3, int len3) {
	constexpr unsigned sha1_size = 20;

	if(hash_size > (int)sha1_size)
		std::fill((uint8_t*)hash_return, (uint8_t*)hash_return + sha1_size, 0);

	CryptoPP::SHA1 sha1;
	sha1.Update((const uint8_t*)v1, len1);
	sha1.Update((const uint8_t*)v2, len2);
	sha1.Update((const uint8_t*)v3, len3);
	sha1.TruncatedFinal((uint8_t*)hash_return, std::min(sha1.DigestSize(), sha1_size));
}

int dht_random_bytes(void *buf, size_t size) {
	CryptoPP::AutoSeededRandomPool rng;
	rng.GenerateBlock((uint8_t*)buf, size);
	return size;
}

} /* extern "C" */
