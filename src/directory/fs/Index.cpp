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
#include "Index.h"
#include "FSDirectory.h"
#include "Meta.pb.h"
#include "../../../contrib/crypto/Base32.h"

namespace librevault {

Index::Index(FSDirectory& dir, Session& session) : log_(spdlog::get("Librevault")), dir_(dir) {
	bool db_path_created = fs::create_directories(dir_.db_path().parent_path());
	log_->debug() << "Database directory: " << dir_.db_path().parent_path() << (db_path_created ? " created" : "");

	if(fs::exists(dir_.db_path()))
		log_->debug() << "Opening SQLite3 DB: " << dir_.db_path();
	else
		log_->debug() << "Creating new SQLite3 DB: " << dir_.db_path();
	db_ = std::make_unique<SQLiteDB>(dir_.db_path());
	db_->exec("PRAGMA foreign_keys = ON;");

	db_->exec("CREATE TABLE IF NOT EXISTS files (path_hmac BLOB PRIMARY KEY NOT NULL, mtime INTEGER NOT NULL, meta BLOB NOT NULL, signature BLOB NOT NULL);");
	db_->exec("CREATE TABLE IF NOT EXISTS blocks (encrypted_hash BLOB NOT NULL PRIMARY KEY, blocksize INTEGER NOT NULL, iv BLOB NOT NULL);");
	db_->exec("CREATE TABLE IF NOT EXISTS openfs (block_encrypted_hash BLOB NOT NULL REFERENCES blocks (encrypted_hash) ON DELETE CASCADE ON UPDATE CASCADE, file_path_hmac BLOB NOT NULL REFERENCES files (path_hmac) ON DELETE CASCADE ON UPDATE CASCADE, [offset] INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");

	db_->exec("CREATE TRIGGER IF NOT EXISTS block_deleter DELETE ON openfs BEGIN DELETE FROM blocks WHERE encrypted_hash NOT IN (SELECT block_encrypted_hash FROM openfs); END;");
}

Index::~Index() {}

void Index::put_Meta(std::list<SignedMeta> signed_meta_list) {
	auto raii_lock = SQLiteLock(*db_);
	for(auto signed_meta : signed_meta_list){
		SQLiteSavepoint raii_transaction(*db_, "put_Meta");
		Meta meta; meta.ParseFromArray(signed_meta.meta.data(), signed_meta.meta.size());

		auto file_path_hmac = SQLValue((const uint8_t*)meta.path_hmac().data(), meta.path_hmac().size());
		db_->exec("INSERT OR REPLACE INTO files (path_hmac, mtime, meta, signature) VALUES (:path_hmac, :mtime, :meta, :signature);", {
				{":path_hmac", file_path_hmac},
				{":mtime", (int64_t)meta.mtime()},
				{":meta", SQLValue((uint8_t*)signed_meta.meta.data(), signed_meta.meta.size())},
				{":signature", signed_meta.signature}
		});

		uint64_t offset = 0;
		for(auto block : meta.filemap().blocks()){
			auto block_encrypted_hash = SQLValue((const uint8_t*)block.encrypted_hash().data(), block.encrypted_hash().size());
			db_->exec("INSERT OR IGNORE INTO blocks (encrypted_hash, blocksize, iv) VALUES (:encrypted_hash, :blocksize, :iv);", {
					{":encrypted_hash", block_encrypted_hash},
					{":blocksize", (int64_t)block.blocksize()},
					{":iv", SQLValue((const uint8_t*)block.iv().data(), block.iv().size())}
			});

			db_->exec("INSERT INTO openfs (block_encrypted_hash, file_path_hmac, [offset]) VALUES (:block_encrypted_hash, :file_path_hmac, :offset);", {
					{":block_encrypted_hash", block_encrypted_hash},
					{":file_path_hmac", file_path_hmac},
					{":offset", (int64_t)offset}
			});

			offset += block.blocksize();
		}

		log_->debug() << "Added Meta of " << (std::string)crypto::Base32().to(meta.path_hmac());
	}
}

std::list<Index::SignedMeta> Index::get_Meta(std::string sql, std::map<std::string, SQLValue> values){
	std::list<SignedMeta> result_list;
	for(auto row : db_->exec(sql, values))
		result_list.push_back({row[0], row[1]});
	return result_list;
}
Index::SignedMeta Index::get_Meta(blob path_hmac){
	auto meta_list = get_Meta("SELECT meta, signature FROM files WHERE path_hmac=:path_hmac LIMIT 1", {
			{":path_hmac", path_hmac}
	});

	if(meta_list.empty()) throw no_such_meta();
	return *meta_list.begin();
}
std::list<Index::SignedMeta> Index::get_Meta(int64_t mtime){
	return get_Meta("SELECT meta, signature FROM files WHERE mtime >= :mtime", {{":mtime", mtime}});
}
std::list<Index::SignedMeta> Index::get_Meta(){
	return get_Meta("SELECT meta, signature FROM files");
}

} /* namespace librevault */
