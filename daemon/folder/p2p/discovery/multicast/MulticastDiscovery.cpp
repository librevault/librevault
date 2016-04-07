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
#include "MulticastDiscovery.h"
#include "Client.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "folder/p2p/P2PProvider.h"
#include "folder/p2p/WSServer.h"
#include "MulticastDiscovery.pb.h"
#include "MulticastSender.h"

namespace librevault {

using namespace boost::asio::ip;

MulticastDiscovery::MulticastDiscovery(Client& client, address bind_addr) :
	DiscoveryService(client), socket_(client.network_ios()), bind_addr_(bind_addr) {
	name_ = "MulticastDiscovery";
	client.folder_added_signal.connect(std::bind(&MulticastDiscovery::register_group, this, std::placeholders::_1));
	client.folder_removed_signal.connect(std::bind(&MulticastDiscovery::unregister_group, this, std::placeholders::_1));
}

MulticastDiscovery::~MulticastDiscovery() {
	socket_.set_option(multicast::leave_group(group_.address()));
}

void MulticastDiscovery::register_group(std::shared_ptr<FolderGroup> group_ptr) {
	senders_.insert({group_ptr->secret().get_Hash(), std::make_shared<MulticastSender>(group_ptr, *this)});
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
		log_->info() << log_tag() << "Started Local Peer Discovery on: " << group_;
	}else{
		log_->info() << log_tag() << "Local Peer Discovery is disabled";
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
		log_->debug() << log_tag() << "<== " << node_endpoint;

		std::shared_ptr<MulticastSender> sender;
		auto it = senders_.find(dir_hash);
		if(it != senders_.end()) it->second->consume(node_endpoint, pubkey);
	}else{
		log_->debug() << log_tag() << "Message from " << endpoint_ptr->address() << ": Malformed Protobuf data";
	}

	receive();    // We received message, continue receiving others
}

void MulticastDiscovery::receive() {
	auto endpoint = std::make_shared<udp::endpoint>(socket_.local_endpoint());
	auto buffer = std::make_shared<udp_buffer>();
	socket_.async_receive_from(boost::asio::buffer(buffer->data(), buffer->size()), *endpoint,
	                           std::bind(&MulticastDiscovery::process, this, buffer, std::placeholders::_2, endpoint, std::placeholders::_1));
}

MulticastDiscovery4::MulticastDiscovery4(Client& client) :
	MulticastDiscovery(client, address_v4::any()) {

	socket_.open(boost::asio::ip::udp::v4());

	reload_config();
	start();
}

void MulticastDiscovery4::reload_config() {
	enabled_ = Config::get()->client()["multicast4_enabled"].asBool();
	repeat_interval_ = seconds(Config::get()->client()["multicast4_repeat_interval"].asUInt64());

	auto multicast_group_url = url(Config::get()->client()["multicast4_group"].asString());
	group_ = udp_endpoint(address::from_string(multicast_group_url.host), multicast_group_url.port);
}

MulticastDiscovery6::MulticastDiscovery6(Client& client) :
	MulticastDiscovery(client, address_v6::any()) {

	socket_.open(boost::asio::ip::udp::v6());
	socket_.set_option(boost::asio::ip::v6_only(true));

	reload_config();
	start();
}

void MulticastDiscovery6::reload_config() {
	enabled_ = Config::get()->client()["multicast6_enabled"].asBool();
	repeat_interval_ = seconds(Config::get()->client()["multicast6_repeat_interval"].asUInt64());

	auto multicast_group_url = url(Config::get()->client()["multicast6_group"].asString());
	group_ = udp_endpoint(address::from_string(multicast_group_url.host), multicast_group_url.port);
}

} /* namespace librevault */