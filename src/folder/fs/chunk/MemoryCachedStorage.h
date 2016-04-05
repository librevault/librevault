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
#include "src/pch.h"
#include "AbstractStorage.h"

namespace librevault {

class Client;
// Cache implemented as a simple LRU structure over doubly-linked list and associative container (std::map, in this case)
class MemoryCachedStorage : public AbstractStorage, public Loggable {
public:
	MemoryCachedStorage(FSFolder& dir, ChunkStorage& chunk_storage);
	virtual ~MemoryCachedStorage() {}

	bool have_chunk(const blob& ct_hash) const noexcept;
	std::shared_ptr<blob> get_chunk(const blob& ct_hash) const;
	void put_chunk(const blob& ct_hash, std::shared_ptr<blob> data);
	void remove_chunk(const blob& ct_hash) noexcept;

private:
	using ct_hash_data_type = std::pair<blob, std::shared_ptr<blob>>;
	using list_iterator_type = std::list<ct_hash_data_type>::iterator;

	mutable std::list<ct_hash_data_type> cache_list_;
	std::map<blob, list_iterator_type> cache_iteraror_map_;

	bool overflow() const;
};

} /* namespace librevault */
