/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "../types.h"
#include <Meta.pb.h>
#include "../../contrib/lvsqlite3/SQLiteWrapper.h"

#include <boost/lockfree/queue.hpp>
#include <boost/asio.hpp>

#include <list>
#include "Key.h"

namespace librevault {
namespace syncfs {

struct SignedMeta {
	std::string meta;
	blob signature;
};

/**
 * SyncFS class.
 * Uses io_service for concurrency.
 * Thread safety: thread-safe after construction.
 */
class SyncFS {
	const Key key;

	fs::path open_path, db_path, block_path;

	mutable std::shared_ptr<SQLiteDB> directory_db;

	std::shared_ptr<boost::asio::io_service> internal_io_service;
	boost::asio::io_service::strand internal_io_strand;

	boost::asio::io_service& external_io_service;
	boost::asio::io_service::strand external_io_strand;	// We need this, as executing multiple SQLite statements (CrUD) is dangerous even is serialized mode.
public:
	class error : public std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
	};
	class no_such_meta : public error {
		no_such_meta() : error("No such Meta"){}
	};

// Methods
private:
	std::string make_Meta(const std::string& file_path);
	SignedMeta sign(std::string meta) const;
public:
	SyncFS(boost::asio::io_service& io_service, Key key,
			fs::path open_path,
			fs::path db_path = open_path / ".librevault" / "directory.db",
			fs::path block_path = open_path / ".librevault");
	virtual ~SyncFS();

	std::string make_portable_path(const fs::path& path) const;

	// Indexing functions
	void queue_index(const std::set<std::string> file_path);
	void queue_index(const std::string& file_path){queue_index({file_path});};

	void wipe_index();

	// File listers
	/**
	 * Returns file names that are actually present in OpenFS.
	 * @return
	 */
	std::set<std::string> get_openfs_file_list();
	/**
	 * Returns file names that are mentioned in index. They may be present, maybe not.
	 * @return
	 */
	std::set<std::string> get_index_file_list();

	// Meta functions
	void put_Meta(SignedMeta signed_meta);
	void put_Meta(std::list<SignedMeta> signed_meta);

	SignedMeta get_Meta(blob path_hmac);
	std::list<SignedMeta> get_Meta(int64_t mtime);
	std::list<SignedMeta> get_Meta();

	bool have_Meta(blob path_hmac);
	void remove_Meta(blob path_hmac);

	// Block functions
	blob get_block(const blob& block_hash);
	void put_block(const blob& block_hash, const blob& data);

	blob get_encblock(const blob& block_hash);
	void put_encblock(const blob& block_hash, const blob& data);
};

} /* namespace syncfs */
} /* namespace librevault */
