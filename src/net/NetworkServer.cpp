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
#include "NetworkServer.h"

namespace librevault {

NetworkServer::NetworkServer(const boost::property_tree::ptree& properties, const boost::property_tree::ptree& properties_global, boost::asio::io_service& io_service) :
		properties(properties), properties_global(properties_global), io_service(io_service), tcp_acceptor(io_service) {
	tcp_acceptor.open(boost::asio::ip::tcp::v6());

}

NetworkServer::~NetworkServer() {
	// TODO Auto-generated destructor stub
}

} /* namespace librevault */
