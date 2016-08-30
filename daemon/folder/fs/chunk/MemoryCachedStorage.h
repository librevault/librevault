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
#include "pch.h"
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
