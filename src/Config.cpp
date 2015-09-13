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
#include "Config.h"
#include "Session.h"

namespace librevault {

Config::Config(Session& session) : session_(session), log_(session_.log()) {
	fs::ifstream options_fs(session_.config_path(), std::ios::binary);
	if(!options_fs){
		log_->info() << "Writing default configuration to: " << session_.config_path();
		// TODO: Create default configuration file
	}

	boost::property_tree::info_parser::read_info(options_fs, config_ptree_);
	log_->info() << "Configuration file loaded: " << session_.config_path();
}

Config::~Config() {
	fs::ofstream options_fs(session_.config_path(), std::ios::trunc | std::ios::binary);
	boost::property_tree::info_parser::write_info(options_fs, config_ptree_);
}

} /* namespace librevault */
