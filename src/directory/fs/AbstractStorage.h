/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "../../pch.h"
#include "../../contrib/crypto/SHA3.h"

namespace librevault {

class AbstractStorage {
public:
	class no_such_block : public std::runtime_error {
	public:
		no_such_block() : std::runtime_error("Requested Block not found"){}
	};

	AbstractStorage(){};
	virtual ~AbstractStorage(){};

	bool verify_encblock(const blob& block_hash, const blob& data){
		return crypto::SHA3(28).compute(data) == crypto::BinaryArray(block_hash);
	}
	virtual blob get_encblock(const blob& block_hash) = 0;
};

} /* namespace librevault */
