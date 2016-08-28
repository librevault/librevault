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
#include "pch.h"
#include "Index.h"
#include "folder/fs/chunk/EncStorage.h"
#include "folder/fs/chunk/OpenStorage.h"
#include <librevault/Meta.h>

namespace librevault {

class FSFolder;
class Indexer : public Loggable {
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("Indexer error") {}
	};

	struct abort_index : error {
		abort_index(const char* what) : error(what) {}
	};

	struct unsupported_filetype : abort_index {
		unsupported_filetype() : abort_index("File type is unsuitable for indexing. Only Files, Directories and Symbolic links are supported") {}
	};

	Indexer(FSFolder& dir, Client& client);
	virtual ~Indexer() {}

	// Index manipulation
	void index(const std::string& file_path) noexcept;

	void async_index(const std::string& file_path);
	void async_index(const std::set<std::string>& file_path);

	// Meta functions
	SignedMeta make_Meta(const std::string& relpath);

	/* Getters */
	bool is_indexing() const {return indexing_now_ != 0;}

private:
	FSFolder& dir_;

	const Secret& secret_;
	Index& index_;

	Client& client_;

	/* Status */
	std::atomic_uint indexing_now_;
	std::set<std::string> index_queue_;
	std::mutex index_queue_mtx_;

	/* File analyzers */
	Meta::Type get_type(const boost::filesystem::path& path);
	void update_fsattrib(const Meta& old_meta, Meta& new_meta, const boost::filesystem::path& path);
	void update_chunks(const Meta& old_meta, Meta& new_meta, const boost::filesystem::path& path);
	Meta::Chunk populate_chunk(const Meta& new_meta, const blob& data, const std::map<blob, blob>& pt_hmac__iv);
};

} /* namespace librevault */
