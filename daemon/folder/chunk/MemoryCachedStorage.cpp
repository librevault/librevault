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
#include "MemoryCachedStorage.h"
#include "ChunkStorage.h"

namespace librevault {

MemoryCachedStorage::MemoryCachedStorage(QObject* parent) : QObject(parent) {}

bool MemoryCachedStorage::have_chunk(const blob& ct_hash) const noexcept {
	return cache_iteraror_map_.find(ct_hash) != cache_iteraror_map_.end();
}

std::shared_ptr<blob> MemoryCachedStorage::get_chunk(const blob& ct_hash) const {
	QMutexLocker lk(&cache_lock_);

	auto it = cache_iteraror_map_.find(ct_hash);
	if(it == cache_iteraror_map_.end()) {
		throw ChunkStorage::no_such_chunk();
	}else{
		cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
		return it->second->second;
	}
}

void MemoryCachedStorage::put_chunk(const blob& ct_hash, std::shared_ptr<blob> data) {
	QMutexLocker lk(&cache_lock_);

	auto it = cache_iteraror_map_.find(ct_hash);
	if(it != cache_iteraror_map_.end()) {
		cache_list_.erase(it->second);
		cache_iteraror_map_.erase(it);
	}

	cache_list_.push_front(ct_hash_data_type(ct_hash, data));
	cache_iteraror_map_[ct_hash] = cache_list_.begin();

	if(overflow()) {
		auto last = cache_list_.end();
		last--;
		cache_iteraror_map_.erase(last->first);
		cache_list_.pop_back();
	}
}

void MemoryCachedStorage::remove_chunk(const blob& ct_hash) noexcept {
	QMutexLocker lk(&cache_lock_);

	auto iterator_to_iterator = cache_iteraror_map_.find(ct_hash);
	if(iterator_to_iterator != cache_iteraror_map_.end()) {
		cache_list_.erase(iterator_to_iterator->second);
		cache_iteraror_map_.erase(iterator_to_iterator);
	}
}

bool MemoryCachedStorage::overflow() const {
	return cache_iteraror_map_.size() > 10; // TODO: implement other cache clearing strategy.
}

} /* namespace librevault */
