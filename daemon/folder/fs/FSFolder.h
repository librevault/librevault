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
#include "folder/AbstractFolder.h"

#include "control/Config.h"
#include "control/FolderParams.h"
#include "folder/FolderGroup.h"

#include <librevault/SignedMeta.h>

namespace librevault {

class FolderGroup;

class IgnoreList;
class Index;
class Indexer;
class AutoIndexer;
class ChunkStorage;

class FSFolder : public AbstractFolder, public std::enable_shared_from_this<FSFolder> {
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("FSFolder error") {}
	};
	struct status_t {
		uint64_t byte_size = 0;
		uint64_t file_count = 0;
		bool is_indexing = false;
	};

	/* Components */
	std::unique_ptr<IgnoreList> ignore_list;
	std::unique_ptr<Index> index;

	std::unique_ptr<ChunkStorage> chunk_storage;

	std::unique_ptr<Indexer> indexer;
	std::unique_ptr<AutoIndexer> auto_indexer;

	/* Constructors */
	FSFolder(FolderGroup& group, Client& client);
	virtual ~FSFolder();

	/* Actions */
	bool have_meta(const Meta::PathRevision& path_revision) noexcept;
	SignedMeta get_meta(const Meta::PathRevision& path_revision);
	void put_meta(SignedMeta smeta, bool fully_assembled = false);

	bool have_chunk(const blob& ct_hash) const noexcept;
	blob get_chunk(const blob& ct_hash);
	void put_chunk(const blob& ct_hash, const blob& chunk);

	bitfield_type get_bitfield(const Meta::PathRevision& path_revision);

	/* Makers */
	std::string normalize_path(const fs::path& abspath) const;
	fs::path absolute_path(const std::string& normpath) const;

	status_t status();

	/* Getters */
	FolderGroup& group() {return group_;}
	const FolderGroup& group() const {return group_;}

	const FolderParams& params() const {return group_.params();}
	const Secret& secret() const {return params().secret;}
	const fs::path& path() const {return params().path;}
	const fs::path& system_path() const {return params().system_path;}

private:
	FolderGroup& group_;
};

} /* namespace librevault */
