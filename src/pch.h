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

// Cryptodiff
#include <cryptodiff.h>

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
#include <boost/dynamic_bitset.hpp>
#include <boost/endian/arithmetic.hpp>

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
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <shared_mutex>
#include <queue>
#include <regex>
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

using byte = uint8_t;
using blob = std::vector<byte>;

using bitfield_type = boost::dynamic_bitset<uint8_t>;

using logger_ptr = std::shared_ptr<spdlog::logger>;

} /* namespace librevault */
