/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "p2p/Discovery.h"
#include "p2p/ConnectionManager.h"
#include "syncfs/Key.h"
#include "../contrib/dir_monitor/include/dir_monitor.hpp"

#include <spdlog/spdlog.h>

#include "NodeKey.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/signals2.hpp>

#include <memory>
#include <thread>
#include <vector>
#include <unordered_map>
#include <map>

namespace librevault {

namespace fs = boost::filesystem;
using boost::asio::io_service;
using boost::property_tree::ptree;

class Directory;

class Session {
	std::shared_ptr<spdlog::logger> log;

	io_service ios;
	std::unique_ptr<io_service::work> work_lock;
	std::vector<std::thread> worker_threads;

	std::unique_ptr<p2p::ConnectionManager> cm;

	std::unique_ptr<NodeKey> node_key;

	fs::path options_path;	// Config directory

	boost::asio::dir_monitor monitor;

	// Program options
	ptree options;

	std::map<blob, std::shared_ptr<Directory>> hash_dir;
	std::map<fs::path, std::shared_ptr<Directory>> path_dir;

	struct Signals {
		enum SignalType {
			UNKNOWN = 0,
			DIRECTORY_ADDED,
			DIRECTORY_REMOVED,
			DIRECTORY_CHANGED
		};

		boost::signals2::signal<void(std::shared_ptr<Directory>, SignalType)> directory;
	} signals;
public:
	Session(fs::path glob_config_path);
	virtual ~Session();

	static fs::path get_default_config_path();

	void run();
	void shutdown();

	void add_directory(ptree dir_options);
	void remove_directory(std::shared_ptr<Directory> dir_ptr);

	void start_directories();
	void start_monitor();

	//NodeKey& get_nodekey(){return *node_key;};
	p2p::ConnectionManager& get_cm(){return *cm;}
	boost::asio::dir_monitor& get_monitor(){return monitor;}
	Signals& get_signals(){return signals;}
	io_service& get_ios(){return ios;}
	ptree& get_options(){return options;}
};

} /* namespace librevault */
