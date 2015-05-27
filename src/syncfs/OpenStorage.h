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

#include "EncStorage.h"
#include "../types.h"

namespace librevault {
namespace syncfs {

class OpenStorage {
	std::shared_ptr<SQLiteDB> directory_db;
	const Key& key;
	EncStorage& enc_storage;

	const fs::path& open_path;
	const fs::path& block_path;

	std::pair<blob, blob> get_both_blocks(const blob& block_hash);	// Returns std::pair(plaintext, encrypted)
public:
	OpenStorage(const Key& key, std::shared_ptr<SQLiteDB> directory_db, EncStorage& enc_storage, const fs::path& open_path, const fs::path& block_path);
	virtual ~OpenStorage();

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
};

} /* namespace syncfs */
} /* namespace librevault */
