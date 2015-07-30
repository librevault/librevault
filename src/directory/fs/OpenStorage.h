/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "../../pch.h"
#include "AbstractStorage.h"
#include "../Key.h"
#include "Index.h"
#include "EncStorage.h"

namespace librevault {

class OpenStorage : public AbstractStorage {
public:
	OpenStorage(const Key& key, Index& index, EncStorage& enc_storage, fs::path open_path, fs::path asm_path);
	virtual ~OpenStorage();

	// Path constructor
	std::string make_relpath(const fs::path& path) const;

	blob get_encblock(const blob& block_hash);
	blob get_block(const blob& block_hash);

	// File assembler
	/**
	 * Searches for unassembled files ready to be assembled and assembles them
	 * @param delete_blocks
	 */
	void assemble(bool delete_blocks = true);
	/**
	 * Tries to split unencrypted file into separate encrypted blocks, pushing them into EncStorage.
	 * @param file_path
	 * @param delete_file
	 */
	void disassemble(const std::string& file_path, bool delete_file = true);

	std::set<std::string> open_files();	// Returns file names that are actually present in OpenFS.
	std::set<std::string> indexed_files();	// Returns file names that are mentioned in index. They may be present, maybe not.
	std::set<std::string> all_files();	// Sum of above

	fs::path open_path() const {return open_path_;}
	fs::path asm_path() const {return asm_path_;}

private:
	std::shared_ptr<spdlog::logger> log_;

	const Key& key_;
	Index& index_;
	EncStorage& enc_storage_;

	fs::path open_path_, asm_path_;

	std::pair<blob, blob> get_both_blocks(const blob& block_hash);	// Returns std::pair(plaintext, encrypted)
	std::string get_path(const blob& path_hmac);
};

} /* namespace librevault */
