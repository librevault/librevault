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
#include "../version.h"
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>
#include <functional>

namespace librevault {
namespace discovery {

NodeDB::NodeDB(io_service& ios, ptree& options) : ios(ios), options(options) {

}

void NodeDB::start(){
	std::function<void(ptree)> handler = [this](ptree node_description){handle_discovery(node_description);};
	if(options.get<bool>("discovery.multicast4.enabled"))
		try {
			multicast4 = std::make_unique<Multicast4>(ios, options, handler); multicast4->start();
		}catch(std::runtime_error& e){
			BOOST_LOG_TRIVIAL(info) << "Cannot initialize UDPv4 multicast discovery";
		}
	if(options.get<bool>("discovery.multicast6.enabled"))
		try {
			multicast6 = std::make_unique<Multicast6>(ios, options, handler); multicast6->start();
		}catch(std::runtime_error& e){
			BOOST_LOG_TRIVIAL(info) << "Cannot initialize UDPv6 multicast discovery";
		}
}

void NodeDB::stop(){
	multicast4.reset();
	multicast6.reset();
}

void NodeDB::handle_discovery(ptree node_description){
	BOOST_LOG_TRIVIAL(trace) << node_description.get<uint16_t>("port");
}

} /* namespace discovery */
} /* namespace librevault */
