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
#include <librevault/Secret.h>
#include "src/folder/fs/Index.h"
#include "EncStorage.h"

namespace librevault {

class FSFolder;
class OpenStorage : public AbstractStorage, public Loggable {
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("OpenStorage error") {}
	};

	OpenStorage(FSFolder& dir, ChunkStorage& chunk_storage);
	virtual ~OpenStorage() {}

	bool have_chunk(const blob& ct_hash) const noexcept;
	std::shared_ptr<blob> get_chunk(const blob& ct_hash) const;

private:
	const Secret& secret_;
};

} /* namespace librevault */
