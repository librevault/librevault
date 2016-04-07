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
#include "src/pch.h"
#include "src/folder/p2p/discovery/DiscoveryInstance.h"

namespace librevault {

class MulticastDiscovery;

class MulticastSender : public DiscoveryInstance, public Loggable {
public:
	MulticastSender(std::weak_ptr<FolderGroup> group, MulticastDiscovery& service);

	void consume(const tcp_endpoint& node_endpoint, const blob& pubkey);

private:
	boost::asio::steady_timer repeat_timer_;
	std::mutex repeat_timer_mtx_;

	mutable std::string message_;
	std::string get_message() const;

	void maintain_requests(const boost::system::error_code& ec = boost::system::error_code());
};

} /* namespace librevault */