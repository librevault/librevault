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

#include "../contrib/dir_monitor/include/dir_monitor.hpp"
#include "nodedb/NodeDB.h"

#include "sync/OpenFSBlockStorage.h"

#include <cryptodiff.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <array>

void print_endpoint(boost::asio::ip::udp::endpoint endpoint){
	std::cout << endpoint << std::endl;
}

int main(int argc, char** argv){
	//std::array<uint8_t, 32> key; std::copy(argv[2], argv[2]+32, &*key.begin());

	//boost::asio::io_service io_service;
	//boost::asio::dir_monitor dir_monitor(io_service);
	//dir_monitor.add_directory("./test");
	//for(;;){
	//	boost::asio::dir_monitor_event ev = dir_monitor.monitor();
	//	if(ev.type == boost::asio::dir_monitor_event::added){
	//		cryptodiff::FileMap map(key);
	//		std::ifstream istream(ev.path.c_str(), std::ios_base::in | std::ios_base::binary);
	//		map.create(istream);
	//	}

	//	std::cout << ev << std::endl;
	//}

	const char* key_c = "12345678901234567890123456789012";
	std::array<uint8_t, 32> key; std::copy(key_c, key_c+32, &*key.begin());
	librevault::OpenFSBlockStorage open_fs_block_storage("/home/gamepad/synced", "/home/gamepad/.librevault/dir.db", key);
	open_fs_block_storage.init();
	open_fs_block_storage.index();

	boost::asio::io_service ios;
	boost::property_tree::ptree pt;
	pt.put("nodedb.udplpdv4.repeat_interval", 30);
	pt.put("nodedb.udplpdv4.multicast_port", 28914);
	pt.put("nodedb.udplpdv4.multicast_ip", "239.192.152.144");
	pt.put("nodedb.udplpdv4.bind_ip", "0.0.0.0");
	pt.put("nodedb.udplpdv4.bind_port", 28914);

	pt.put("nodedb.udplpdv6.repeat_interval", 30);
	pt.put("nodedb.udplpdv6.multicast_port", 28914);
	pt.put("nodedb.udplpdv6.multicast_ip", "ff08::BD02");
	pt.put("nodedb.udplpdv6.bind_ip", "0::0");
	pt.put("nodedb.udplpdv6.bind_port", 28914);

	pt.put("net.port", 30);
	pt.put("net.tcp", true);
	pt.put("net.utp", true);

	librevault::NodeDB node_db(pt.get_child("nodedb"), pt, ios);
	node_db.start_discovery();
	ios.run();

	return 0;
}
