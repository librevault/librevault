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

#include <boost/property_tree/ptree.hpp>
#include <chrono>

namespace librevault {
namespace p2p {

using boost::property_tree::ptree;
using std::chrono::seconds;
using blob = std::vector<uint8_t>;

class Node {
public:
	enum Quality {SUPERIOR, GOOD, QUESTIONABLE, BAD};
private:
	ptree node;
	blob id;

	Quality quality = GOOD;
public:
	Node();
	virtual ~Node();

	blob get_id() const {return id;};
	Quality get_quality() const {return quality;}
};

} /* namespace overlay */
} /* namespace librevault */
