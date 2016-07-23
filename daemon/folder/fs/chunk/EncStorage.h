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
#include "pch.h"
#include "AbstractStorage.h"

namespace librevault {

class Client;
class FSFolder;
class EncStorage : public AbstractStorage, public Loggable {
public:
	EncStorage(FSFolder& dir, ChunkStorage& chunk_storage);
	virtual ~EncStorage() {}

	bool have_chunk(const blob& ct_hash) const noexcept;
	std::shared_ptr<blob> get_chunk(const blob& ct_hash) const;
	void put_chunk(const blob& ct_hash, const fs::path& chunk_location);
	void remove_chunk(const blob& ct_hash);

private:
	std::string make_chunk_ct_name(const blob& ct_hash) const noexcept;
	fs::path make_chunk_ct_path(const blob& ct_hash) const noexcept;
};

} /* namespace librevault */
