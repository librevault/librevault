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
#include "Config.h"

namespace librevault {

Config::Config(Loggable& parent_loggable, fs::path config_path) : Loggable(parent_loggable, "Config"), config_path_(std::move(config_path)) {
	fs::ifstream options_fs(config_path_, std::ios::binary);
	if(!options_fs){
		log_->info() << "Writing default configuration to: " << config_path_;
		// TODO: Create default configuration file
	}

	boost::property_tree::info_parser::read_info(options_fs, config_ptree_);
	log_->info() << "Configuration file loaded: " << config_path_;
}

Config::~Config() {
	fs::ofstream options_fs(config_path_, std::ios::trunc | std::ios::binary);
	boost::property_tree::info_parser::write_info(options_fs, config_ptree_);
}

} /* namespace librevault */
