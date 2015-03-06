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
#ifndef SRC_VERSION_H_
#define SRC_VERSION_H_

#include <cstdint>

const uint8_t version_major = 0;
const uint8_t version_minor = 1;
const uint16_t version_patch = 0;

constexpr const char* get_name() {
	return "LibreVault";
}

constexpr uint32_t get_version() {
	return (version_major << 24) & (version_minor << 16) & version_patch;
}

inline std::string get_version_string() {
	std::ostringstream os;
	os << get_name() << " " << version_major << "." << version_minor << "." << version_patch;
	return os.str();
}

#endif /* SRC_VERSION_H_ */
