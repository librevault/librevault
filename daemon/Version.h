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
#pragma once
#include <string>

namespace librevault {

class Version {
public:
	Version();

	std::string name() const {return name_;}
	std::string version_string() const {return version_string_;}
	std::string user_agent() const {return name() + "/" + version_string_;}

	static Version current() {static Version current; return current;}
protected:
	const std::string name_;
	const std::string version_string_;
};

} /* namespace librevault */
