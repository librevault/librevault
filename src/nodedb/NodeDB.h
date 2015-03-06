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
#ifndef SRC_NODEDB_NODEDB_H_
#define SRC_NODEDB_NODEDB_H_

#include "UDPLPD.pb.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>
#include <map>

namespace librevault {
class UDPLPD;

struct Node {
	std::string appname;
	bool tcp, utp;
};

class NodeDB {
	const boost::property_tree::ptree& properties;
	const boost::property_tree::ptree& properties_global;
	boost::asio::io_service& io_service;

	using node_addr_t = std::pair<boost::asio::ip::address, uint16_t>;
	std::map<node_addr_t, Node> node_map;

	std::unique_ptr<UDPLPD> udplpdv4, udplpdv6;
public:
	NodeDB(const boost::property_tree::ptree& properties, const boost::property_tree::ptree& properties_global, boost::asio::io_service& io_service);
	virtual ~NodeDB();

	void start_discovery();

	void process_UDPLPD_s(const boost::asio::ip::address& from_ip, uint16_t from_port, const UDPLPD_s& udplpd_s);
};

} /* namespace librevault */

#endif /* SRC_NODEDB_NODEDB_H_ */
