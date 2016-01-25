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
#include "PortMappingService.h"
#include "src/util/Loggable.h"

namespace librevault {

class Client;

class P2PProvider;

class NATPMPService : public PortMappingService, public Loggable {
public:
	NATPMPService(Client& client, P2PProvider& provider);

	void set_enabled(bool enabled);
	void set_lifetime(std::chrono::seconds lifetime);

protected:
	Client& client_;
	P2PProvider& provider_;

	// Config values
	bool enabled_;
	std::chrono::seconds lifetime_;

	// Weak timer
	boost::asio::system_timer maintain_timer_;
	std::mutex maintain_timer_mtx_;

	void maintain_mapping(const boost::system::error_code& error = boost::system::error_code());
};

} /* namespace librevault */
