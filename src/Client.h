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
#include "Version.h"
#include "util/Config.h"
#include "util/Loggable.h"
#include "util/multi_io_service.h"

namespace librevault {

class Exchanger;
class Client : public Loggable {
public:
	Client(std::map<std::string, docopt::value> args);
	virtual ~Client();

	void init_log();

	void run();
	void shutdown();
	void restart();

	Exchanger& exchanger(){return *exchanger_;}

	const Version& version() const {return version_;}
	ptree& config() {return config_->config();}

	io_service& ios() {return etc_ios_->ios();}
	io_service& network_ios() {return network_ios_->ios();}
	io_service& dir_monitor_ios() {return etc_ios_->ios();}

	fs::path appdata_path() const {return appdata_path_;}
	fs::path config_path() const {return config_path_;}
	fs::path log_path() const {return log_path_;}
	fs::path key_path() const {return key_path_;}
	fs::path cert_path() const {return cert_path_;}
private:
	Version version_;	// Application name and version information (probably, it will contain application path and signature later)
	std::unique_ptr<Config> config_;	// Configuration

	// Asynchronous/multithreaded operation
	io_service main_loop_ios_;
	std::unique_ptr<multi_io_service> network_ios_;
	std::unique_ptr<multi_io_service> dir_monitor_ios_;
	std::unique_ptr<multi_io_service> etc_ios_;

	// Components
	std::unique_ptr<Exchanger> exchanger_;

	// Paths
	fs::path default_appdata_path();
	fs::path appdata_path_, config_path_, log_path_, key_path_, cert_path_;
};

} /* namespace librevault */
