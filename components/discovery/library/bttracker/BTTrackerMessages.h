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
#include "../btcompat.h"
#include <QtGlobal>
#include <boost/endian/arithmetic.hpp>

namespace librevault {
namespace bttracker {

enum class Action : quint32 {
	ACTION_CONNECT=0, ACTION_ANNOUNCE=1, ACTION_SCRAPE=2, ACTION_ERROR=3, ACTION_ANNOUNCE6=4, ACTION_NONE=255
};

enum class Event : quint32 {
	EVENT_NONE=0, EVENT_COMPLETED=1, EVENT_STARTED=2, EVENT_STOPPED=3
};

#pragma pack(push, 1)
struct conn_req {
	quint64 connection_id_;
	boost::endian::big_uint32_t action_;
	quint32 transaction_id_;
	///
};
struct conn_rep {
	boost::endian::big_uint32_t action_;
	quint32 transaction_id_;
	///
	quint64 connection_id_;
};

struct announce_req {
	quint64 connection_id_;
	boost::endian::big_uint32_t action_;
	quint32 transaction_id_;
	///
	btcompat::info_hash info_hash_;
	btcompat::peer_id peer_id_;
	boost::endian::big_uint64_t downloaded_;
	boost::endian::big_uint64_t left_;
	boost::endian::big_uint64_t uploaded_;
	boost::endian::big_uint32_t event_ = (int32_t)Event::EVENT_NONE;
	boost::endian::big_uint32_t ip_ = 0;
	quint32 key_;
	boost::endian::big_uint32_t num_want_;
	boost::endian::big_uint16_t port_;
	boost::endian::big_uint16_t extensions_ = 0;
};

struct announce_req6 {
	quint64 connection_id_;
	boost::endian::big_uint32_t action_;
	quint32 transaction_id_;
	///
	btcompat::info_hash info_hash_;
	btcompat::peer_id peer_id_;
	boost::endian::big_uint64_t downloaded_;
	boost::endian::big_uint64_t left_;
	boost::endian::big_uint64_t uploaded_;
	boost::endian::big_uint32_t event_ = (int32_t)Event::EVENT_NONE;
	std::array<uint8_t, 16> ip_;
	quint32 key_;
	boost::endian::big_uint32_t num_want_;
	boost::endian::big_uint16_t port_;
	boost::endian::big_uint16_t extensions_ = 0;
};

struct announce_rep {
	boost::endian::big_uint32_t action_;
	quint32 transaction_id_;
	///
	boost::endian::big_uint32_t interval_;
	boost::endian::big_uint32_t leechers_;
	boost::endian::big_uint32_t seeders_;
};
#pragma pack(pop)

} /* namespace bttracker */
} /* namespace librevault */
