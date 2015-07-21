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

// dir_monitor
#include "../contrib/dir_monitor/include/dir_monitor.hpp"

// spdlog
#include <spdlog/spdlog.h>

// OpenSSL
#include <openssl/pem.h>
#include <openssl/x509.h>

// Crypto++
#include <cryptopp/aes.h>
#include <cryptopp/ccm.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/integer.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha3.h>

// Boost
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/system_timer.hpp>

#include <boost/endian/arithmetic.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/lexical_cast.hpp>

//---------FOR REMOVAL----------
#include <boost/log/sinks.hpp>
#include <boost/log/trivial.hpp>
//------------------------------

#include <boost/predef.h>

#include <boost/program_options.hpp>

#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/signals2.hpp>

// Standard C++ Libraries
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// OS-dependent
#if BOOST_OS_LINUX
#include <sys/types.h>
#endif
#if BOOST_OS_LINUX || BOOST_OS_UNIX || BOOST_OS_BSD
#include <pwd.h>
#include <unistd.h>
#endif

namespace librevault {

namespace fs = boost::filesystem;
using blob = std::vector<uint8_t>;
using boost::asio::io_service;
using boost::property_tree::ptree;

using boost::asio::ip::address;
using boost::asio::ip::address_v4;
using boost::asio::ip::address_v6;

using udp_endpoint = boost::asio::ip::udp::endpoint;
using tcp_endpoint = boost::asio::ip::tcp::endpoint;

using udp_socket = boost::asio::ip::udp::socket;
using tcp_socket = boost::asio::ip::tcp::socket;

using ssl_socket = boost::asio::ssl::stream<tcp_socket>;

using byte = uint8_t;

} /* namespace librevault */
