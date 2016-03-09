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
#include "MulticastSender.h"
#include "MulticastDiscovery.pb.h"
#include "src/Client.h"
#include "src/folder/FolderGroup.h"
#include "src/folder/p2p/P2PProvider.h"
#include "src/folder/p2p/WSServer.h"

namespace librevault {

using namespace boost::asio::ip;

MulticastSender::MulticastSender(std::weak_ptr<FolderGroup> group, MulticastDiscovery& service) :
	DiscoveryInstance(group, service), Loggable("MulticastSender"),
	repeat_timer_(service.client().network_ios()) {

	maintain_requests();
}

void MulticastSender::consume(const tcp_endpoint& node_endpoint, const blob& pubkey) {
	auto group_ptr = group_.lock();

	if(group_ptr)
		service_.add_node(node_endpoint, pubkey, group_ptr);
}

std::string MulticastSender::get_message() const {
	if(message_.empty()) {
		auto group_ptr = std::shared_ptr<FolderGroup>(group_);

		if(!group_ptr) return std::string();

		protocol::MulticastDiscovery message;
		message.set_port(service_.client().p2p_provider()->ws_server()->local_endpoint().port());   // Damn, this is painful
		message.set_dir_hash(group_ptr->secret().get_Hash().data(), group_ptr->secret().get_Hash().size());
		message.set_pubkey(service_.client().p2p_provider()->node_key().public_key().data(),
			service_.client().p2p_provider()->node_key().public_key().size());

		message_ = message.SerializeAsString();
	}
	return message_;
}

void MulticastSender::maintain_requests(const boost::system::error_code& ec) {
	if(ec == boost::asio::error::operation_aborted) return;

	if(repeat_timer_mtx_.try_lock()) {
		std::unique_lock<decltype(repeat_timer_mtx_)> repeat_timer_lk(repeat_timer_mtx_, std::adopt_lock);

		MulticastDiscovery& service = dynamic_cast<MulticastDiscovery&>(service_);
		service.socket_.async_send_to(boost::asio::buffer(get_message()), service.group_, std::bind([](){}));
		log_->debug() << log_tag() << "==> " << service.group_;

		repeat_timer_.expires_from_now(service.repeat_interval_);
		repeat_timer_.async_wait(std::bind(&MulticastSender::maintain_requests, this, std::placeholders::_1));
	}
}

} /* namespace librevault */