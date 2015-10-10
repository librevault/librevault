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

class FSDirectory;
class OpenStorage : public AbstractStorage {
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("OpenStorage error") {}
	};

	struct assemble_error : error {
		assemble_error() : error("Error during assembling file") {}
	};

	OpenStorage(FSDirectory& dir, Session& session);
	virtual ~OpenStorage();

	// Path constructor
	std::string make_relpath(const fs::path& path) const;
	blob make_path_id(const std::string& relpath) const;

	blob get_encblock(const blob& encrypted_data_hash);
	blob get_block(const blob& encrypted_data_hash);

	// File assembler
	/**
	 * Searches for unassembled files ready to be assembled and assembles them
	 * @param delete_blocks
	 */
	void assemble(const Meta& meta, bool delete_blocks = true);
	/**
	 * Tries to split unencrypted file into separate encrypted blocks, pushing them into EncStorage.
	 * @param file_path
	 * @param delete_file
	 */
	//void disassemble(const std::string& file_path, bool delete_file = true);

	bool is_skipped(const std::string& relpath) const;

	const std::set<std::string>& skip_files() const;

	std::set<std::string> open_files();	// Returns file names that are actually present in OpenFS.
	std::set<std::string> indexed_files();	// Returns file names that are mentioned in index. They may be present, maybe not.
	std::set<std::string> pending_files();	// Files, ready to be reindexed (sum of above, except deleted)
	std::set<std::string> all_files();	// Sum of all

private:
	std::shared_ptr<spdlog::logger> log_;
	FSDirectory& dir_;

	const Key& key_;
	Index& index_;
	EncStorage& enc_storage_;

	mutable std::set<std::string> skip_files_;

	std::pair<blob, blob> get_both_blocks(const blob& encrypted_data_hash);	// Returns std::pair(plaintext, encrypted)
	std::string get_path(const blob& path_id);
};

} /* namespace librevault */
