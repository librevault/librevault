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

#include "bttracker/UDPTrackerConnection.h"

#include "../version.h"
#include "../net/parse_url.h"
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>
#include <functional>

namespace librevault {
namespace p2p {

NodeDB::NodeDB(io_service& ios, Signals& signals, ptree& options) : ios(ios), signals(signals), options(options) {}

void NodeDB::start_lsd_announcer(){
	std::function<void(ptree)> handler = [this](ptree node_description){handle_discovery(node_description);};
	if(options.get<bool>("discovery.multicast4.enabled"))
		try {
			multicast4 = std::make_unique<Multicast4>(ios, options, handler); multicast4->start();
		}catch(std::runtime_error& e){
			BOOST_LOG_TRIVIAL(info) << "Cannot initialize UDPv4 multicast discovery" << e.what();
		}
	if(options.get<bool>("discovery.multicast6.enabled"))
		try {
			multicast6 = std::make_unique<Multicast6>(ios, options, handler); multicast6->start();
		}catch(std::runtime_error& e){
			BOOST_LOG_TRIVIAL(info) << "Cannot initialize UDPv6 multicast discovery" << e.what();
		}
}

void NodeDB::start_tracker_announcer(){
	if(options.get<bool>("discovery.bttracker.enabled")){
		auto all_trackers = options.get_child("discovery.bttracker").equal_range("tracker");
		for(auto& it = all_trackers.first; it != all_trackers.second; it++){
			try {
				url tracker_address = parse_url(it->second.get_value<std::string>());
				TrackerConnection* tracker_ptr;

				if(tracker_address.scheme == "udp")
					tracker_ptr = new UDPTrackerConnection(tracker_address, ios, signals, options);
				else if(tracker_address.scheme == "http")
					throw std::runtime_error("HTTP trackers not implemented yet. UDP trackers only supported");
				else
					throw std::runtime_error("Unknown tracker scheme");
				torrent_trackers.emplace_back(tracker_ptr);
				tracker_ptr->start();
			}catch(std::runtime_error& e){
				BOOST_LOG_TRIVIAL(info) << "Cannot initialize BitTorrent tracker discovery";
			}
		}
	}else
		BOOST_LOG_TRIVIAL(info) << "BitTorrent tracker discovery is disabled";
}

void NodeDB::stop(){
	multicast4.reset();
	multicast6.reset();
	torrent_trackers.clear();
}

void NodeDB::handle_discovery(ptree node_description){
	BOOST_LOG_TRIVIAL(trace) << node_description.get<uint16_t>("port");
}

} /* namespace discovery */
} /* namespace librevault */
