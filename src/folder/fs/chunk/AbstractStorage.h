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
#include <librevault/Meta.h>
#include "src/util/Loggable.h"

namespace librevault {

class FSFolder;
class ChunkStorage;
class AbstractStorage {
public:
	AbstractStorage(FSFolder& dir, ChunkStorage& chunk_storage);
	virtual ~AbstractStorage() {};

	bool verify_chunk(const blob& ct_hash, const blob& chunk_pt, Meta::StrongHashType strong_hash_type) const {
		return ct_hash == Meta::Chunk::compute_strong_hash(chunk_pt, strong_hash_type);
	}
	virtual std::shared_ptr<blob> get_chunk(const blob& ct_hash) const = 0;

protected:
	FSFolder& dir_;
	ChunkStorage& chunk_storage_;
};

} /* namespace librevault */
