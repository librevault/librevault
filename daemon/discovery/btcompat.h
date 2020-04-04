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
#include "librevault/util/blob.h"
#include <QByteArray>
#include <QHostAddress>
#include <QtEndian>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/endian/arithmetic.hpp>
#include <array>

namespace librevault {
namespace btcompat {

using asio_endpoint = boost::asio::ip::tcp::endpoint;
using asio_address_v4 = boost::asio::ip::address_v4;
using asio_address_v6 = boost::asio::ip::address_v6;

using info_hash = std::array<uint8_t, 20>;
using peer_id = std::array<uint8_t, 20>;

#pragma pack(push, 1)
struct compact_endpoint4 {
	std::array<uint8_t, 4> ip4;
	quint16 port;
};
struct compact_endpoint6 {
	std::array<uint8_t, 16> ip6;
	quint16 port;
};
#pragma pack(pop)

// Check sizes
static_assert(sizeof(compact_endpoint4) == 6, "compact_endpoint4 size is incorrect");
static_assert(sizeof(compact_endpoint6) == 18, "compact_endpoint6 size is incorrect");

// Function declaration
inline info_hash getInfoHash(const QByteArray& folderid) {
	info_hash ih; std::copy(folderid.begin(), folderid.begin()+std::min(ih.size(), (size_t)folderid.size()), ih.begin());
	return ih;
}

inline peer_id get_peer_id(QByteArray digest) {
	peer_id pid;

	std::string az_id = Config::get()->getGlobal("bttracker_azureus_id").toString().toStdString();
	az_id.resize(8);

	digest = digest.leftJustified(pid.size() - az_id.size(), 0, true);

	std::copy(az_id.begin(), az_id.end(), pid.begin());
	std::copy(digest.begin(), digest.end(), pid.begin() + az_id.size());

	return pid;
}

inline asio_endpoint parse_compact_endpoint(const compact_endpoint4& compact_endpoint) {
	return asio_endpoint(asio_address_v4(compact_endpoint.ip4), compact_endpoint.port);
};
inline asio_endpoint parse_compact_endpoint(const compact_endpoint6& compact_endpoint) {
	return asio_endpoint(asio_address_v6(compact_endpoint.ip6), compact_endpoint.port);
};
inline asio_endpoint parse_compact_endpoint4(const void* data) {
	return btcompat::parse_compact_endpoint(*(reinterpret_cast<const btcompat::compact_endpoint4*>(data)));
};
inline asio_endpoint parse_compact_endpoint6(const void* data) {
	return btcompat::parse_compact_endpoint(*(reinterpret_cast<const btcompat::compact_endpoint6*>(data)));
};

inline std::list<asio_endpoint> parse_compact_endpoint4_list(const void* data, size_t size) {
	std::list<asio_endpoint> endpoints;
	for(const uint8_t* data_cur = (const uint8_t*)data; data_cur+6 <= ((const uint8_t*)data+size); data_cur += 6)
		endpoints.push_back(parse_compact_endpoint4(data));
	return endpoints;
}

inline std::list<asio_endpoint> parse_compact_endpoint6_list(const void* data, size_t size) {
	std::list<asio_endpoint> endpoints;
	for(const uint8_t* data_cur = (const uint8_t*)data; data_cur+18 <= ((const uint8_t*)data+size); data_cur += 18)
		endpoints.push_back(parse_compact_endpoint6(data));
	return endpoints;
}

inline std::pair<QHostAddress, quint16> parseCompactEndpoint(const compact_endpoint4& compact_endpoint) {
	return {QHostAddress(*reinterpret_cast<const quint32*>(compact_endpoint.ip4.data())), qFromBigEndian(compact_endpoint.port)};
};

inline std::pair<QHostAddress, quint16> parseCompactEndpoint(const compact_endpoint6& compact_endpoint) {
	return {QHostAddress(compact_endpoint.ip6.data()), qFromBigEndian(compact_endpoint.port)};
};

inline std::pair<QHostAddress, quint16> parseCompactEndpoint4(const void* data) {
	return btcompat::parseCompactEndpoint(*(reinterpret_cast<const btcompat::compact_endpoint4*>(data)));
};
inline std::pair<QHostAddress, quint16> parseCompactEndpoint6(const void* data) {
	return btcompat::parseCompactEndpoint(*(reinterpret_cast<const btcompat::compact_endpoint6*>(data)));
};

inline QList<std::pair<QHostAddress, quint16>> parseCompactEndpoint4List(QByteArray data) {
	QList<std::pair<QHostAddress, quint16>> endpoints;
	for(const char* data_cur = data.data(); data_cur+6 <= (data.data()+data.size()); data_cur += 6)
		endpoints.push_back(parseCompactEndpoint4(data.data()));
	return endpoints;
}

inline QList<std::pair<QHostAddress, quint16>> parseCompactEndpoint6List(QByteArray data) {
	QList<std::pair<QHostAddress, quint16>> endpoints;
	for(const char* data_cur = data.data(); data_cur+18 <= (data.data()+data.size()); data_cur += 18)
		endpoints.push_back(parseCompactEndpoint6(data.data()));
	return endpoints;
}

} /* namespace btcompat */
} /* namespace librevault */
