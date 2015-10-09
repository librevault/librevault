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
#include <string>
#include <cstdint>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace librevault {

std::string size_to_string(double bytes) {
	const double kib = 1024;
	const double mib = 1024 * kib;
	const double gib = 1024 * mib;
	const double tib = 1024 * gib;

	const double kb = 1000;
	const double mb = 1000 * kib;
	const double gb = 1000 * mib;
	const double tb = 1000 * gib;

	std::ostringstream os; os << std::fixed << std::setprecision(2);

	if(bytes < kb)
		os << bytes << " B";
	else if(bytes < mb)
		os << (double)bytes/kib << " kB";
	else if(bytes < gb)
		os << (double)bytes/mib << " MB";
	else if(bytes < tb)
		os << (double)bytes/gib << " GB";
	else
		os << (double)bytes/tib << " TB";
	return os.str();
}

} /* namespace librevault */
