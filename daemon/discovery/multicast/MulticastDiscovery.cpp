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
#include "MulticastDiscovery.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "MulticastDiscovery.pb.h"
#include "MulticastSender.h"
#include <util/log.h>
#include <boost/asio/ip/multicast.hpp>
#include <boost/asio/ip/v6_only.hpp>

namespace librevault {

using namespace boost::asio::ip;

MulticastDiscovery::MulticastDiscovery(DiscoveryService& parent, io_service& io_service, NodeKey& node_key, address bind_addr) :
	DiscoverySubService(parent, io_service, "Multicast"), node_key_(node_key), socket_(io_service), bind_addr_(bind_addr) {}

MulticastDiscovery::~MulticastDiscovery() {
	if(socket_.is_open())
		socket_.set_option(multicast::leave_group(group_.address()));
}

void MulticastDiscovery::register_group(std::shared_ptr<FolderGroup> group_ptr) {
	senders_.insert({group_ptr->secret().get_Hash(), std::make_shared<MulticastSender>(group_ptr, *this, io_service_, node_key_)});
}

void MulticastDiscovery::unregister_group(std::shared_ptr<FolderGroup> group_ptr) {
	senders_.erase(group_ptr->secret().get_Hash());
}

void MulticastDiscovery::start() {
	socket_.set_option(multicast::join_group(group_.address()));
	socket_.set_option(multicast::enable_loopback(false));
	socket_.set_option(udp::socket::reuse_address(true));

	socket_.bind(udp::endpoint(bind_addr_, group_.port()));

	receive();

	if(enabled_) {
		LOGI("Started Local Peer Discovery on: " << group_);
	}else{
		LOGI("Local Peer Discovery is disabled");
	}
}

void MulticastDiscovery::process(std::shared_ptr<udp_buffer> buffer, size_t size, std::shared_ptr<udp::endpoint> endpoint_ptr, const boost::system::error_code& ec) {
	if(ec == boost::asio::error::operation_aborted) return;

	protocol::MulticastDiscovery message;
	if(enabled_ && message.ParseFromArray(buffer->data(), size)) {
		uint16_t port = uint16_t(message.port());
		blob dir_hash = blob(message.dir_hash().begin(), message.dir_hash().end());
		blob pubkey = blob(message.pubkey().begin(), message.pubkey().end());

		tcp_endpoint node_endpoint(endpoint_ptr->address(), port);
		LOGD("<== " << node_endpoint);

		std::shared_ptr<MulticastSender> sender;
		auto it = senders_.find(dir_hash);
		if(it != senders_.end()) it->second->consume(node_endpoint, pubkey);
	}else{
		LOGD("Message from " << endpoint_ptr->address() << ": Malformed Protobuf data");
	}

	receive();    // We received message, continue receiving others
}

void MulticastDiscovery::receive() {
	auto endpoint = std::make_shared<udp::endpoint>(socket_.local_endpoint());
	auto buffer = std::make_shared<udp_buffer>();
	socket_.async_receive_from(boost::asio::buffer(buffer->data(), buffer->size()), *endpoint,
	                           std::bind(&MulticastDiscovery::process, this, buffer, std::placeholders::_2, endpoint, std::placeholders::_1));
}

MulticastDiscovery4::MulticastDiscovery4(DiscoveryService& parent, io_service& io_service, NodeKey& node_key) :
	MulticastDiscovery(parent, io_service, node_key, address_v4::any()) {

	reload_config();
}

void MulticastDiscovery4::reload_config() {
	enabled_ = Config::get()->globals()["multicast4_enabled"].asBool();
	repeat_interval_ = std::chrono::seconds(Config::get()->globals()["multicast4_repeat_interval"].asUInt64());

	auto multicast_group_url = url(Config::get()->globals()["multicast4_group"].asString());
	group_ = udp_endpoint(address::from_string(multicast_group_url.host), multicast_group_url.port);

	if(enabled_ && !socket_.is_open()) {
		try {
			socket_.open(boost::asio::ip::udp::v4());
			start();
		}catch(std::exception& e){
			LOGE("Could not initialize IPv4 local discovery");
		}
	}
}

MulticastDiscovery6::MulticastDiscovery6(DiscoveryService& parent, io_service& io_service, NodeKey& node_key) :
	MulticastDiscovery(parent, io_service, node_key, address_v6::any()) {

	reload_config();
}

void MulticastDiscovery6::reload_config() {
	enabled_ = Config::get()->globals()["multicast6_enabled"].asBool();
	repeat_interval_ = std::chrono::seconds(Config::get()->globals()["multicast6_repeat_interval"].asUInt64());

	auto multicast_group_url = url(Config::get()->globals()["multicast6_group"].asString());
	group_ = udp_endpoint(address::from_string(multicast_group_url.host), multicast_group_url.port);

	if(enabled_ && !socket_.is_open()) {
		try {
			socket_.open(boost::asio::ip::udp::v6());
			socket_.set_option(boost::asio::ip::v6_only(true));
			start();
		}catch(std::exception& e){
			LOGE("Could not initialize IPv6 local discovery");
		}
	}
}

} /* namespace librevault */
