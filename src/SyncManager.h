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

#include "discovery/NodeDB.h"
#include "syncfs/Key.h"
#include "../contrib/dir_monitor/include/dir_monitor.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
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

class SyncManager {
	io_service ios;
	std::unique_ptr<io_service::work> work_lock;
	std::vector<std::thread> worker_threads;

	fs::path options_path;	// Config directory

	boost::asio::dir_monitor monitor;

	// Program options
	ptree options;

	std::map<syncfs::Key, std::shared_ptr<Directory>> key_dir;
	std::map<fs::path, std::shared_ptr<Directory>> path_dir;

	std::unique_ptr<discovery::NodeDB> nodedb;
public:
	SyncManager(fs::path glob_config_path);
	virtual ~SyncManager();

	static fs::path get_default_config_path();

	void run();
	void shutdown();

	void init_directories();
	void add_directory(ptree dir_options);

	void start_monitor();
};

} /* namespace librevault */
