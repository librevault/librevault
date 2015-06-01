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
#include "EncStorage.h"
#include "OpenStorage.h"

#include "../types.h"
#include "Key.h"

#include <boost/asio.hpp>

#include <list>
#include <set>

namespace librevault {
namespace syncfs {

struct SignedMeta {
	blob meta;
	blob signature;
};

/**
 * SyncFS class.
 * Uses io_service for concurrency.
 * Thread safety: thread-safe after construction.
 */
class SyncFS {
	const Key key;

	const fs::path open_path, db_path, block_path;

	std::shared_ptr<SQLiteDB> directory_db;

	// Boost asio io_service
	std::shared_ptr<boost::asio::io_service> internal_io_service;
	boost::asio::io_service::strand internal_io_strand;

	boost::asio::io_service& external_io_service;
	boost::asio::io_service::strand external_io_strand;	// We need this, as executing multiple SQLite statements (CrUD) is dangerous even is serialized mode.

	// Components
	std::unique_ptr<EncStorage> enc_storage;
	std::unique_ptr<OpenStorage> open_storage;
public:
	class error : public std::runtime_error {
	public:
		error(const char* what) : std::runtime_error(what) {}
	};
	class no_such_meta : public error {
	public:
		no_such_meta() : error("Requested Meta not found"){}
	};
	class no_such_block : public error {
	public:
		no_such_block() : error("Requested Block not found"){}
	};

// Methods
private:
	blob make_Meta(const std::string& file_path);
	SignedMeta sign(const blob& meta) const;

	std::list<SignedMeta> get_Meta(std::string sql, std::map<std::string, SQLValue> values = std::map<std::string, SQLValue>());
public:
	SyncFS(boost::asio::io_service& io_service, Key key,
			fs::path open_path,
			fs::path block_path,
			fs::path db_path);
	virtual ~SyncFS();

	std::string make_portable_path(const fs::path& path) const;

	// Indexing functions
	void index(const std::set<std::string> file_path);
	void index(const std::string& file_path){index({file_path});};

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
	void put_Meta(std::list<SignedMeta> signed_meta_list);
	void put_Meta(SignedMeta signed_meta){put_Meta(std::list<SignedMeta>{signed_meta});}

	SignedMeta get_Meta(blob path_hmac);
	std::list<SignedMeta> get_Meta(int64_t mtime);
	std::list<SignedMeta> get_Meta();

	// Block functions
	blob get_block(const blob& block_hash);
	void put_block(const blob& block_hash, const blob& data);

	blob get_encblock(const blob& block_hash);
	void put_encblock(const blob& block_hash, const blob& data);

	// File assembler/disassembler invokation functions
	void disassemble(const std::string& file_path, bool delete_file = true){open_storage->disassemble(file_path, delete_file);}
	void assemble(bool delete_blocks = true){open_storage->assemble(delete_blocks);}
};

} /* namespace syncfs */
} /* namespace librevault */
