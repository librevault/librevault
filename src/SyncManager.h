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

#include "syncfs/Key.h"
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <memory>
#include <thread>
#include <vector>

namespace librevault {

using boost::asio::io_service;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

class SyncManager {
	class Directory {
	public:
		Directory();
		~Directory();
	};

	io_service ios;
	std::unique_ptr<io_service::work> work_lock;
	std::vector<std::thread> worker_threads;
	int thread_count;

	fs::path global_config_path;	// Config directory

	// Program options
	po::variables_map options;
	po::options_description program_options_desc, core_options_desc;

	fs::path get_default_config_path();
public:
	SyncManager();
	SyncManager(int argc, char** argv);
	virtual ~SyncManager();

	void run();
	void shutdown();
};

} /* namespace librevault */
