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
class FileAssembler : public Loggable {
public:
	struct error : std::runtime_error {
		error(const std::string& what) : std::runtime_error(what) {}
		error() : error("FileAssembler error") {}
	};

	FileAssembler(FSFolder& dir, ChunkStorage& chunk_storage);
	virtual ~FileAssembler() {}

	blob get_chunk_pt(const blob& ct_hash) const;

	// File assembler
	void assemble(const Meta& meta);
	//void disassemble(const std::string& file_path, bool delete_file = true);

private:
	FSFolder& dir_;
	ChunkStorage& chunk_storage_;
	const Secret& secret_;
	Index& index_;

	void assemble_deleted(const Meta& meta);
	void assemble_symlink(const Meta& meta);
	void assemble_directory(const Meta& meta);
	void assemble_file(const Meta& meta);

	void apply_attrib(const Meta& meta);
};

} /* namespace librevault */
