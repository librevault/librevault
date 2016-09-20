/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#pragma once
#include "control/Config.h"
#include "util/network.h"
#include <boost/endian/arithmetic.hpp>

namespace librevault {

namespace btcompat {

using info_hash = std::array<uint8_t, 20>;
using peer_id = std::array<uint8_t, 20>;

#pragma pack(push, 1)
struct compact_endpoint4 {
	std::array<uint8_t, 4> ip4;
	boost::endian::big_uint16_t port;
};
struct compact_endpoint6 {
	std::array<uint8_t, 16> ip6;
	boost::endian::big_uint16_t port;
};
#pragma pack(pop)

// Check sizes
static_assert(sizeof(compact_endpoint4) == 6, "compact_endpoint4 size is incorrect");
static_assert(sizeof(compact_endpoint6) == 18, "compact_endpoint6 size is incorrect");

// Function declaration
inline info_hash get_info_hash(const blob& dir_hash) {
	info_hash ih; std::copy(dir_hash.begin(), dir_hash.begin()+std::min(ih.size(), dir_hash.size()), ih.begin());
	return ih;
}

inline peer_id get_peer_id(const blob& node_pubkey) {
	peer_id pid;

	std::string az_id = Config::get()->globals()["bttracker_azureus_id"].asString();
	az_id.resize(8);

	auto pubkey_bytes_left = pid.size() - az_id.size();

	std::copy(az_id.begin(), az_id.end(), pid.begin());
	std::copy(node_pubkey.begin(),
		node_pubkey.begin() + pubkey_bytes_left, pid.begin() + az_id.size());

	return pid;
}

inline tcp_endpoint parse_compact_endpoint(const compact_endpoint4& compact_endpoint) {
	return tcp_endpoint(address_v4(compact_endpoint.ip4), compact_endpoint.port);
};
inline tcp_endpoint parse_compact_endpoint(const compact_endpoint6& compact_endpoint) {
	return tcp_endpoint(address_v6(compact_endpoint.ip6), compact_endpoint.port);
};
inline tcp_endpoint parse_compact_endpoint4(const void* data) {
	return btcompat::parse_compact_endpoint(*(reinterpret_cast<const btcompat::compact_endpoint4*>(data)));
};
inline tcp_endpoint parse_compact_endpoint6(const void* data) {
	return btcompat::parse_compact_endpoint(*(reinterpret_cast<const btcompat::compact_endpoint6*>(data)));
};
inline blob make_compact_endpoint(const tcp_endpoint& endpoint) {
	blob compact_endpoint(18);

	if(endpoint.address().is_v6()) {
		((compact_endpoint6*)(compact_endpoint.data()))->ip6 = endpoint.address().to_v6().to_bytes();
		((compact_endpoint6*)(compact_endpoint.data()))->port = endpoint.port();
	}else if(endpoint.address().is_v4()) {
		compact_endpoint.resize(6);
		((compact_endpoint4*)(compact_endpoint.data()))->ip4 = endpoint.address().to_v4().to_bytes();
		((compact_endpoint4*)(compact_endpoint.data()))->port = endpoint.port();
	}

	return compact_endpoint;
};

} /* namespace btcompat */
} /* namespace librevault */
