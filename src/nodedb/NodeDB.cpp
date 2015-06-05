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
#include "NodeDB.h"
#include "UDPLPD.h"
#include "../version.h"
#include <boost/log/trivial.hpp>

namespace librevault {

NodeDB::NodeDB(const boost::property_tree::ptree& properties, const boost::property_tree::ptree& properties_global, boost::asio::io_service& io_service) :
		properties(properties),	properties_global(properties_global), io_service(io_service) {
	udplpdv4 = std::unique_ptr<UDPLPD>(new UDPLPD(io_service, properties.get_child("udplpdv4"), this));
	udplpdv6 = std::unique_ptr<UDPLPD>(new UDPLPD(io_service, properties.get_child("udplpdv6"), this));
}

NodeDB::~NodeDB() {}

void NodeDB::start_discovery(){
	UDPLPD_s message;
	message.set_appname(get_version_string());
	message.set_port(properties_global.get<uint16_t>("net.port"));
	message.set_tcp(properties_global.get<bool>("net.tcp"));
	message.set_utp(properties_global.get<bool>("net.utp"));
	udplpdv4->set_multicast_message(message);
	udplpdv6->set_multicast_message(message);

	udplpdv4->start();
	udplpdv6->start();
}

void NodeDB::process_UDPLPD_s(const boost::asio::ip::address& from_ip, uint16_t from_port, const UDPLPD_s& udplpd_s) {
	node_addr_t node_addr = {from_ip, from_port};
	auto found_it = node_map.find(node_addr);
	if(found_it == node_map.end()){
		Node new_node;
		new_node.appname = udplpd_s.appname();
		new_node.tcp = udplpd_s.tcp();
		new_node.utp = udplpd_s.utp();

		node_map.insert({node_addr, new_node});
		BOOST_LOG_TRIVIAL(debug) << "Found new node: " << boost::asio::ip::udp::endpoint(from_ip, from_port);
	}
}

} /* namespace librevault */
