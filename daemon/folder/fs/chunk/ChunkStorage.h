/* Copyright (C) 2016 Alexander Shishenko <GamePad64@gmail.com>
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
#include <boost/signals2/signal.hpp>
#include <librevault/Meta.h>
#include <librevault/util/bitfield_convert.h>

namespace librevault {

class Client;
class FSFolder;

class MemoryCachedStorage;
class EncStorage;
class OpenStorage;

class FileAssembler;

class ChunkStorage {
public:
	boost::signals2::signal<void(const blob&)> new_chunk_signal;

	ChunkStorage(FSFolder& dir, Client& client);
	virtual ~ChunkStorage();

	bool have_chunk(const blob& ct_hash) const noexcept ;
	blob get_chunk(const blob& ct_hash);  // Throws AbstractFolder::no_such_chunk
	void put_chunk(const blob& ct_hash, const blob& chunk);

	bitfield_type make_bitfield(const Meta& meta) const noexcept;   // Bulk version of "have_chunk"

	void cleanup(const Meta& meta);

protected:
	FSFolder& dir_;
	Client& client_;

	std::unique_ptr<MemoryCachedStorage> mem_storage;
	std::unique_ptr<EncStorage> enc_storage;
	std::unique_ptr<OpenStorage> open_storage;

	std::unique_ptr<FileAssembler>(file_assembler);
};

} /* namespace librevault */
