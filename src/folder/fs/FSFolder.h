/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include <src/control/Config.h>
#include <librevault/SignedMeta.h>
#include "src/folder/AbstractFolder.h"

namespace librevault {

class FolderGroup;

class IgnoreList;
class Index;
class MemoryCachedStorage;
class EncStorage;
class OpenStorage;
class Indexer;
class AutoIndexer;

class FSFolder : public AbstractFolder, public std::enable_shared_from_this<FSFolder> {
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("FSFolder error") {}
	};

	using FolderConfig = Config::FolderConfig;

	/* Components */
	std::unique_ptr<IgnoreList> ignore_list;
	std::unique_ptr<Index> index;

	std::unique_ptr<MemoryCachedStorage> mem_storage;
	std::unique_ptr<EncStorage> enc_storage;
	std::unique_ptr<OpenStorage> open_storage;

	std::unique_ptr<Indexer> indexer;
	std::unique_ptr<AutoIndexer> auto_indexer;

	/* Constructors */
	FSFolder(FolderConfig folder_config, Client& client);
	virtual ~FSFolder();

	/* Actions */
	bool have_meta(const Meta::PathRevision& path_revision);
	SignedMeta get_meta(const Meta::PathRevision& path_revision);
	std::list<SignedMeta> get_meta_containing(const blob& ct_hash);
	void put_meta(SignedMeta smeta, bool fully_assembled = false);

	blob get_block(const blob& ct_hash, uint32_t offset, uint32_t size);

	bool have_chunk(const blob& ct_hash) const;
	blob get_chunk(const blob& ct_hash);
	void put_chunk(const blob& ct_hash, const blob& chunk);

	bitfield_type get_bitfield(const Meta::PathRevision& path_revision);

	/* Makers */
	std::string make_relpath(const fs::path& path) const;

	/* Getters */
	const FolderConfig& folder_config() const {return folder_config_;}

	const Secret& secret() const {return secret_;}
	std::string name() const;

	const fs::path& open_path() const {return open_path_;}
	const fs::path& chunk_path() const {return chunk_path_;}
	const fs::path& db_path() const {return db_path_;}
	const fs::path& asm_path() const {return asm_path_;}

private:
	const FolderConfig folder_config_;
	const Secret secret_;
	const fs::path open_path_, chunk_path_, db_path_, asm_path_;	// Paths

	bitfield_type make_bitfield(const Meta& meta) const;
};

} /* namespace librevault */
