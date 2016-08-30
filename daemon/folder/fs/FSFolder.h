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
#include "folder/AbstractFolder.h"

#include "control/Config.h"
#include "control/FolderParams.h"
#include "folder/FolderGroup.h"

#include <librevault/SignedMeta.h>
#include <boost/filesystem/path.hpp>

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
	void put_chunk(const blob& ct_hash, const boost::filesystem::path& chunk_location);

	bitfield_type get_bitfield(const Meta::PathRevision& path_revision);

	/* Makers */
	std::string normalize_path(const boost::filesystem::path& abspath) const;
	boost::filesystem::path absolute_path(const std::string& normpath) const;

	/* Getters */
	FolderGroup& group() {return group_;}
	const FolderGroup& group() const {return group_;}

	const FolderParams& params() const {return group_.params();}
	const Secret& secret() const {return params().secret;}
	const boost::filesystem::path& path() const {return params().path;}
	const boost::filesystem::path& system_path() const {return params().system_path;}

private:
	FolderGroup& group_;
};

} /* namespace librevault */
