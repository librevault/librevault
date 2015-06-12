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
#include "Options.h"
#include "syncfs/SyncFS.h"
#include "../contrib/dir_monitor/include/dir_monitor.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>

namespace librevault {

using boost::property_tree::ptree;
using boost::asio::io_service;
namespace fs = boost::filesystem;

class Directory {
	syncfs::Key key;
	ptree dir_options;
	std::unique_ptr<syncfs::SyncFS> syncfs_ptr;

	boost::asio::dir_monitor& monitor;
	io_service& ios;

	Options& options;
public:
	Directory(io_service& ios, boost::asio::dir_monitor& monitor, ptree dir_options, Options& options);
	~Directory();

	std::string make_relative_path(const fs::path& path) const {return syncfs_ptr->make_portable_path(path);}
	void handle_modification(const boost::asio::dir_monitor_event& ev);
};

} /* namespace librevault */
