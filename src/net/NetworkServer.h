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
#ifndef SRC_NET_NETWORKDAEMON_H_
#define SRC_NET_NETWORKSERVER_H_

#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>

namespace librevault {

class NetworkServer {
	const boost::property_tree::ptree& properties, properties_global;
	boost::asio::io_service& io_service;

	boost::asio::ip::tcp::acceptor tcp_acceptor;
public:
	NetworkServer(const boost::property_tree::ptree& properties, const boost::property_tree::ptree& properties_global, boost::asio::io_service& io_service);
	virtual ~NetworkServer();
};

} /* namespace librevault */

#endif /* SRC_NET_NETWORKDAEMON_H_ */
