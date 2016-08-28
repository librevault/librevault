/* Copyright (C) 2016 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace librevault {

using boost::asio::io_service;

using boost::asio::ip::address;
using boost::asio::ip::address_v4;
using boost::asio::ip::address_v6;

using udp_resolver = boost::asio::ip::udp::resolver;
using tcp_resolver = boost::asio::ip::tcp::resolver;

using udp_endpoint = boost::asio::ip::udp::endpoint;
using tcp_endpoint = boost::asio::ip::tcp::endpoint;

using udp_socket = boost::asio::ip::udp::socket;
using tcp_socket = boost::asio::ip::tcp::socket;

using ssl_socket = boost::asio::ssl::stream<tcp_socket>;
using ssl_context = boost::asio::ssl::context;

} /* namespace librevault */
