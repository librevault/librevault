/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "SyncManager.h"
#include "syncfs/Key.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

int main(int argc, char** argv){
	po::options_description allowed_options("Allowed Options");
	allowed_options.add_options()
		("help,h", "Display help message")
		("config,c", po::value<fs::path>()->default_value(librevault::SyncManager::get_default_config_path()))
		("gen-key", "Generate Owner key")
	;

	po::variables_map variables;
	po::store(po::parse_command_line(argc, argv, allowed_options), variables);
	po::notify(variables);

	if(variables.count("help") > 0){
		std::cout << allowed_options;
		return 0;
	}
	if(variables.count("gen-key") > 0){
		librevault::syncfs::Key k;
		std::cout << k;
		return 0;
	}

	librevault::SyncManager sync_manager(variables["config"].as<fs::path>());
	sync_manager.run();

	return 0;
}
