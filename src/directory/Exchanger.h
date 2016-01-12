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
#include "src/util/Loggable.h"
#include "src/control/Config.h"

namespace librevault {

class Client;

// Providers
class P2PProvider;
class CloudProvider;

// Discovery services
class StaticDiscovery;
class MulticastDiscovery;
class BTTrackerDiscovery;

// NAT Traversal services
class NATPMPService;

class ExchangeGroup;

class Exchanger : protected Loggable {
public:
	Exchanger(Client& client);
	virtual ~Exchanger();

	void register_group(std::shared_ptr<ExchangeGroup> group_ptr);
	void unregister_group(std::shared_ptr<ExchangeGroup> group_ptr);

	std::shared_ptr<ExchangeGroup> get_group(const blob& hash);

	uint16_t public_port() const {return public_port_;}
	void set_public_port(uint16_t port);

	std::list<std::shared_ptr<ExchangeGroup>> groups() const;

	P2PProvider* p2p_provider();
private:
	Client& client_;

	// Remote
	std::unique_ptr<P2PProvider> p2p_provider_;
	//std::unique_ptr<CloudProvider> cloud_provider_;

	std::map<blob, std::shared_ptr<ExchangeGroup>> hash_group_;

	std::unique_ptr<NATPMPService> natpmp_;
	uint16_t public_port_;

	std::unique_ptr<StaticDiscovery> static_discovery_;
	std::unique_ptr<MulticastDiscovery> multicast4_, multicast6_;
	std::unique_ptr<BTTrackerDiscovery> bttracker_;

	std::string log_tag() const {return "Exchanger";}

	void add_directory(const Config::FolderConfig& folder_config);
};

} /* namespace librevault */
