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
#include <util/fs.h>
#include <util/network.h>
#include <librevault/Meta.h>
#include <librevault/util/bitfield_convert.h>
#include <boost/filesystem/path.hpp>
#include <boost/signals2/signal.hpp>

namespace librevault {

class FSFolder;

class MemoryCachedStorage;
class EncStorage;
class OpenStorage;

class FileAssembler;

class ChunkStorage {
public:
	boost::signals2::signal<void(const blob&)> new_chunk_signal;

	ChunkStorage(FSFolder& dir, io_service& ios);
	virtual ~ChunkStorage();

	bool have_chunk(const blob& ct_hash) const noexcept ;
	blob get_chunk(const blob& ct_hash);  // Throws AbstractFolder::no_such_chunk
	void put_chunk(const blob& ct_hash, const fs::path& chunk_location);

	bitfield_type make_bitfield(const Meta& meta) const noexcept;   // Bulk version of "have_chunk"

	void cleanup(const Meta& meta);

protected:
	FSFolder& dir_;

	std::unique_ptr<MemoryCachedStorage> mem_storage;
	std::unique_ptr<EncStorage> enc_storage;
	std::unique_ptr<OpenStorage> open_storage;

	std::unique_ptr<FileAssembler>(file_assembler);
};

} /* namespace librevault */
