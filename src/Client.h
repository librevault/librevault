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
#include "pch.h"
#pragma once
#include "util/Loggable.h"
#include "util/multi_io_service.h"
#include "src/control/Config.h"

namespace librevault {

class Config;
class ControlServer;

class ExchangeGroup;

// Providers
class P2PProvider;

class CloudProvider;

// Discovery services
class StaticDiscovery;

class MulticastDiscovery;

class BTTrackerDiscovery;

class Client : public Loggable {
public:
	Client(std::map<std::string, docopt::value> args);
	virtual ~Client();

	void run();
	void shutdown();
	void restart();

	Config& config() {return *config_;}

	io_service& ios() {return etc_ios_->ios();}
	io_service& network_ios() {return network_ios_->ios();}
	io_service& dir_monitor_ios() {return etc_ios_->ios();}

	fs::path appdata_path() const {return appdata_path_;}
	fs::path config_path() const {return config_path_;}
	fs::path log_path() const {return log_path_;}
	fs::path key_path() const {return key_path_;}
	fs::path cert_path() const {return cert_path_;}

	// ExchangeGroup
	void register_group(std::shared_ptr<ExchangeGroup> group_ptr);
	void unregister_group(std::shared_ptr<ExchangeGroup> group_ptr);

	std::shared_ptr<ExchangeGroup> get_group(const blob& hash);

	std::list<std::shared_ptr<ExchangeGroup>> groups() const;

	void add_directory(const Config::FolderConfig& folder_config);

	P2PProvider* p2p_provider();
private:
	std::unique_ptr<Config> config_;	// Configuration

	// Asynchronous/multithreaded operation
	io_service main_loop_ios_;
	std::unique_ptr<multi_io_service> network_ios_;
	std::unique_ptr<multi_io_service> dir_monitor_ios_;
	std::unique_ptr<multi_io_service> etc_ios_;

	// Components
	std::unique_ptr<ControlServer> control_server_;

	// Remote
	std::unique_ptr<P2PProvider> p2p_provider_;
	//std::unique_ptr<CloudProvider> cloud_provider_;

	std::map<blob, std::shared_ptr<ExchangeGroup>> hash_group_;

	std::unique_ptr<StaticDiscovery> static_discovery_;
	std::unique_ptr<MulticastDiscovery> multicast4_, multicast6_;
	std::unique_ptr<BTTrackerDiscovery> bttracker_;

	// Paths
	fs::path default_appdata_path();
	fs::path appdata_path_, config_path_, log_path_, key_path_, cert_path_;

	/* Initialization steps */
	void init_log(spdlog::level::level_enum level);
};

} /* namespace librevault */
