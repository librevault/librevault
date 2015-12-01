/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "../pch.h"
#pragma once
#include "Meta.h"
#include "../util/Loggable.h"

namespace librevault {

class Client;
class Exchanger;
class ExchangeGroup;

class AbstractDirectory : protected Loggable {
	friend class ExchangeGroup;
public:
	using bitfield_type = boost::dynamic_bitset<uint8_t>;

	AbstractDirectory(Client& client, Exchanger& exchanger);
	virtual ~AbstractDirectory();

	// AbstractDirectory properties
	virtual std::string name() const = 0;

	// Other functions
	std::string path_id_readable(const blob& path_id) const;
	std::string encrypted_data_hash_readable(const blob& block_id) const;

	blob convert_bitfield(const bitfield_type& bitfield);
	bitfield_type convert_bitfield(blob bitfield);
	// Loggable
	std::string log_tag() const {return std::string("[") + name() + "] ";}

protected:
	Client& client_;
	Exchanger& exchanger_;

	std::weak_ptr<ExchangeGroup> exchange_group_;

	std::map<blob, std::pair<int64_t, AbstractDirectory::bitfield_type>> path_id_info_;
	std::shared_timed_mutex path_id_info_mtx_;

	bool will_accept_meta(const Meta::PathRevision& path_revision);
};

} /* namespace librevault */
