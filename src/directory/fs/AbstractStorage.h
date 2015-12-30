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
#pragma once
#include "../../pch.h"
#include "../../util/Loggable.h"

namespace librevault {

class FSDirectory;
class AbstractStorage {
public:
	AbstractStorage(FSDirectory& dir);
	virtual ~AbstractStorage() {};

	bool verify_block(const blob& encrypted_data_hash, const blob& data, cryptodiff::StrongHashType strong_hash_type) const {
		return encrypted_data_hash == cryptodiff::compute_strong_hash(data, strong_hash_type);
	}
	virtual std::shared_ptr<blob> get_block(const blob& block_hash) const = 0;

protected:
	FSDirectory& dir_;
};

} /* namespace librevault */
