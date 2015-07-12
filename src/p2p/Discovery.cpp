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
#include "Discovery.h"

#include "bttracker/UDPTrackerConnection.h"
#include "../version.h"
#include "../net/parse_url.h"
#include <functional>

namespace librevault {
namespace p2p {

Discovery::Discovery(Session& session) : session(session), log(spdlog::stderr_logger_mt("")) {}

void Discovery::start_lsd_announcer(){
	std::function<void(ptree)> handler = [this](ptree node_description){};
	if(session.get_options().get<bool>("discovery.multicast4.enabled"))
		try {
			multicast4 = std::make_unique<Multicast4>(session.get_ios(), session.get_options(), handler); multicast4->start();
		}catch(std::runtime_error& e){
			log->error() << "Cannot initialize UDPv4 multicast discovery" << e.what();
		}
	if(session.get_options().get<bool>("discovery.multicast6.enabled"))
		try {
			multicast6 = std::make_unique<Multicast6>(session.get_ios(), session.get_options(), handler); multicast6->start();
		}catch(std::runtime_error& e){
			log->error() << "Cannot initialize UDPv6 multicast discovery" << e.what();
		}
}

void Discovery::start_tracker_announcer(){
	if(session.get_options().get<bool>("discovery.bttracker.enabled")){
		auto all_trackers = session.get_options().get_child("discovery.bttracker").equal_range("tracker");
		for(auto& it = all_trackers.first; it != all_trackers.second; it++){
			try {
				url tracker_address = parse_url(it->second.get_value<std::string>());
				TrackerConnection* tracker_ptr;

				if(tracker_address.scheme == "udp")
					tracker_ptr = new UDPTrackerConnection(tracker_address, session);
				else if(tracker_address.scheme == "http")
					throw std::runtime_error("HTTP trackers not implemented yet. UDP trackers supported only");
				else
					throw std::runtime_error("Unknown tracker scheme");
				torrent_trackers.emplace_back(tracker_ptr);
			}catch(std::runtime_error& e){
				log->error() << "Cannot initialize BitTorrent tracker discovery: " << e.what();
			}
		}
	}else
		log->info() << "BitTorrent tracker discovery is disabled";
}

void Discovery::stop(){
	multicast4.reset();
	multicast6.reset();
	torrent_trackers.clear();
}

} /* namespace discovery */
} /* namespace librevault */
