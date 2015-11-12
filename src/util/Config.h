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
#include "../pch.h"
#include "Loggable.h"

namespace librevault {

class Config : protected Loggable {
public:
	Config(LogRoot& log_root, fs::path config_path);
	~Config();

	ptree& config() {return config_ptree_;}
private:
	fs::path config_path_;
	ptree config_ptree_;

	std::string log_tag() const {return "[Config] ";}
};

} /* namespace librevault */
