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
#pragma once

#include "Multicast.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>
#include <map>

namespace librevault {
namespace discovery {

using boost::property_tree::ptree;
using boost::asio::io_service;

class NodeDB {
	io_service& ios;

	using node_addr_t = std::pair<boost::asio::ip::address, uint16_t>;

	std::unique_ptr<Multicast> multicast4, multicast6;
	ptree& options;
public:
	NodeDB(io_service& ios, ptree& options);
	virtual ~NodeDB(){};

	void start();
	void stop();

	void handle_discovery(ptree node_description);
};

} /* namespace discovery */
} /* namespace librevault */
