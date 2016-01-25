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
#include "Index.h"
#include "EncStorage.h"

namespace librevault {

class FSFolder;
class OpenStorage : public AbstractStorage, public Loggable {
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("OpenStorage error") {}
	};

	struct assemble_error : error {
		assemble_error() : error("Error during assembling file") {}
	};

	OpenStorage(FSFolder& dir);
	virtual ~OpenStorage() {}

	// Path constructor
	blob make_path_id(const std::string& relpath) const;

	bool have_block(const blob& encrypted_data_hash) const;
	std::shared_ptr<blob> get_block(const blob& encrypted_data_hash) const;
	blob get_openblock(const blob& encrypted_data_hash) const;

	// File assembler
	/**
	 * Tries to assemble files, indexed by Meta.
	 * @param meta
	 * @param delete_blocks
	 */
	void assemble(const Meta& meta, bool delete_blocks = true);
	/**
	 * Tries to split unencrypted file into separate encrypted blocks, pushing them into EncStorage.
	 * @param file_path
	 * @param delete_file
	 */
	//void disassemble(const std::string& file_path, bool delete_file = true);

	std::set<std::string> open_files();	// Returns file names that are actually present in OpenFS.
	std::set<std::string> indexed_files();	// Returns file names that are mentioned in index. They may be present, maybe not.
	std::set<std::string> pending_files();	// Files, ready to be reindexed (sum of above, except deleted)
	std::set<std::string> all_files();	// Sum of all

private:
	const Secret& secret_;
	Index& index_;
	EncStorage& enc_storage_;

	std::pair<std::shared_ptr<blob>, std::shared_ptr<blob>> get_both_blocks(const blob& encrypted_data_hash) const;	// Returns std::pair(plaintext, encrypted)

	void assemble_deleted(const Meta& meta);
	void assemble_symlink(const Meta& meta);
	void assemble_directory(const Meta& meta);
	void assemble_file(const Meta& meta, bool delete_blocks);

	void apply_attrib(const Meta& meta);
};

} /* namespace librevault */
