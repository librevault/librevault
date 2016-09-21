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
#include "util/log_scope.h"
#include "util/fs.h"
#include "util/network.h"
#include <librevault/SignedMeta.h>
#include <boost/filesystem/path.hpp>
#include <set>
#include <atomic>
#include <mutex>
#include <map>
#include <future>

namespace librevault {

class Index;
class FSFolder;
class Indexer {
	LOG_SCOPE("Indexer");
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

	Indexer(FSFolder& dir, io_service& ios);
	virtual ~Indexer();

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

	io_service& ios_;

	/* Status */
	std::atomic_uint indexing_now_;
	std::map<std::string, std::future<void>> index_queue_;
	std::mutex index_queue_mtx_;
	bool active = true;

	/* File analyzers */
	Meta::Type get_type(const fs::path& path);
	void update_fsattrib(const Meta& old_meta, Meta& new_meta, const fs::path& path);
	void update_chunks(const Meta& old_meta, Meta& new_meta, const fs::path& path);
	Meta::Chunk populate_chunk(const Meta& new_meta, const blob& data, const std::map<blob, blob>& pt_hmac__iv);
};

} /* namespace librevault */
