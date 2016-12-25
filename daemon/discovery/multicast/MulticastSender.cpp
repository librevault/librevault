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
#include "MulticastSender.h"
#include "MulticastDiscovery.pb.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "nodekey/NodeKey.h"
#include <util/log.h>

namespace librevault {

using namespace boost::asio::ip;

MulticastSender::MulticastSender(std::weak_ptr<FolderGroup> group, MulticastDiscovery& service, io_service& io_service, NodeKey& node_key) :
		DiscoveryInstance(group, service, io_service),
		node_key_(node_key),
		repeat_scope_(io_service),
		repeat_timer_(io_service),
		enabled_(static_cast<MulticastDiscovery&>(service_).enabled_) {
	repeat_timer_.tick_signal.connect(repeat_scope_.wrap([this]{maintain_requests();}));
	repeat_timer_.start(service.repeat_interval_, ScopedTimer::RUN_IMMEDIATELY, ScopedTimer::RESET_TIMER, ScopedTimer::NOT_SINGLESHOT);
}

void MulticastSender::consume(const tcp_endpoint& node_endpoint, const blob& pubkey) {
	DiscoveryService::ConnectCredentials cred;
	cred.endpoint = node_endpoint;
	cred.pubkey = pubkey;
	service_.add_node(cred, group_);
}

std::string MulticastSender::get_message() const {
	if(message_.empty()) {
		auto group_ptr = std::shared_ptr<FolderGroup>(group_);

		if(!group_ptr) return std::string();

		protocol::MulticastDiscovery message;
		message.set_port(Config::get()->global_get("p2p_listen").asUInt());
		message.set_dir_hash(group_ptr->hash().data(), group_ptr->hash().size());
		message.set_pubkey(node_key_.public_key().data(), node_key_.public_key().size());

		message_ = message.SerializeAsString();
	}
	return message_;
}

void MulticastSender::maintain_requests() {
	if(!enabled_) return;

	auto& service = static_cast<MulticastDiscovery&>(service_);
	service.socket_.async_send_to(boost::asio::buffer(get_message()), service.group_, std::bind([](){}));
	LOGD("==> " << service.group_);
}

} /* namespace librevault */
