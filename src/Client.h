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
#include "src/folder/FolderGroup.h"

namespace librevault {

class Config;
class ControlServer;

class FolderGroup;

// Providers
class P2PProvider;
class CloudProvider;

class Client : public Loggable {
public:
	Client(std::map<std::string, docopt::value> args);
	virtual ~Client();

	void run();
	void shutdown();

	Config& config() {return *config_;}

	io_service& ios() {return etc_ios_->ios();}
	io_service& network_ios() {return network_ios_->ios();}
	io_service& bulk_ios() {return bulk_ios_->ios();}

	/* Signals */
	boost::signals2::signal<void(std::shared_ptr<FolderGroup>)> folder_added_signal;
	boost::signals2::signal<void(std::shared_ptr<FolderGroup>)> folder_removed_signal;

	// FolderGroup
	void add_folder(Config::FolderConfig folder_config);
	void remove_folder(Secret secret);

	std::shared_ptr<FolderGroup> get_group(const blob& hash);

	std::vector<std::shared_ptr<FolderGroup>> groups() const;

	P2PProvider* p2p_provider();
private:
	std::unique_ptr<Config> config_;	// Configuration

	/* Components */
	std::unique_ptr<ControlServer> control_server_;

	// Remote
	std::unique_ptr<P2PProvider> p2p_provider_;
	//std::unique_ptr<CloudProvider> cloud_provider_;

	std::map<blob, std::shared_ptr<FolderGroup>> hash_group_;

	/* Asynchronous/multithreaded operation */
	io_service main_loop_ios_;
	std::unique_ptr<multi_io_service> network_ios_;
	std::unique_ptr<multi_io_service> bulk_ios_;
	std::unique_ptr<multi_io_service> etc_ios_;

	/* Initialization */
	void init_log(spdlog::level::level_enum level);
};

} /* namespace librevault */
