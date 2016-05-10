/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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

// LVCrypto
#include <librevault/crypto/HMAC-SHA3.h>
#include <librevault/crypto/Base64.h>
#include <librevault/crypto/Base32.h>
#include <librevault/crypto/LuhnModN.h>
#include <librevault/crypto/Hex.h>
#include <librevault/crypto/AES_CBC.h>

// Crypto++
#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>

// Boost
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/system_timer.hpp>

#include <boost/dynamic_bitset.hpp>

#include <boost/endian/arithmetic.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/iostreams/device/mapped_file.hpp>

#include <boost/lexical_cast.hpp>

#include <boost/locale.hpp>

#include <boost/predef.h>

#include <boost/range/adaptor/map.hpp>

#include <boost/signals2.hpp>

// Standard C++ Libraries
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace librevault {

namespace fs = boost::filesystem;

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

using byte = uint8_t;
using blob = std::vector<byte>;

using seconds = std::chrono::seconds;

} /* namespace librevault */
