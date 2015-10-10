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

namespace librevault {

Index::Index(FSDirectory& dir, Session& session) : log_(spdlog::get("Librevault")), dir_(dir) {
	bool db_path_created = fs::create_directories(dir_.db_path().parent_path());
	log_->debug() << dir_.log_tag() << "Database directory: " << dir_.db_path().parent_path() << (db_path_created ? " created" : "");

	if(fs::exists(dir_.db_path()))
		log_->debug() << dir_.log_tag() << "Opening SQLite3 DB: " << dir_.db_path();
	else
		log_->debug() << dir_.log_tag() << "Creating new SQLite3 DB: " << dir_.db_path();
	db_ = std::make_unique<SQLiteDB>(dir_.db_path());
	db_->exec("PRAGMA foreign_keys = ON;");

	db_->exec("CREATE TABLE IF NOT EXISTS files (path_id BLOB PRIMARY KEY NOT NULL, meta BLOB NOT NULL, signature BLOB NOT NULL);");
	db_->exec("CREATE TABLE IF NOT EXISTS blocks (encrypted_data_hash BLOB NOT NULL PRIMARY KEY, blocksize INTEGER NOT NULL, iv BLOB NOT NULL);");
	db_->exec("CREATE TABLE IF NOT EXISTS openfs (encrypted_data_hash BLOB NOT NULL REFERENCES blocks (encrypted_data_hash) ON DELETE CASCADE ON UPDATE CASCADE, path_id BLOB NOT NULL REFERENCES files (path_id) ON DELETE CASCADE ON UPDATE CASCADE, [offset] INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");

	db_->exec("CREATE TRIGGER IF NOT EXISTS block_deleter DELETE ON openfs BEGIN DELETE FROM blocks WHERE encrypted_data_hash NOT IN (SELECT encrypted_data_hash FROM openfs); END;");
}

Index::~Index() {}

void Index::put_Meta(const std::list<SignedMeta>& signed_meta_list) {
	auto raii_lock = SQLiteLock(*db_);
	for(auto signed_meta : signed_meta_list){
		SQLiteSavepoint raii_transaction(*db_, "put_Meta");
		Meta meta; meta.parse(signed_meta.meta);

		db_->exec("INSERT OR REPLACE INTO files (path_id, meta, signature) VALUES (:path_id, :meta, :signature);", {
				{":path_id", meta.path_id()},
				{":meta", signed_meta.meta},
				{":signature", signed_meta.signature}
		});

		uint64_t offset = 0;
		for(auto block : meta.blocks()){
			db_->exec("INSERT OR IGNORE INTO blocks (block_encrypted_hash, blocksize, iv) VALUES (:block_encrypted_hash, :blocksize, :iv);", {
					{":block_encrypted_hash", block.encrypted_data_hash_},
					{":blocksize", (uint64_t)block.blocksize_},
					{":iv", block.iv_}
			});

			db_->exec("INSERT INTO openfs (block_encrypted_hash, path_id, [offset]) VALUES (:block_encrypted_hash, :path_id, :offset);", {
					{":block_encrypted_hash", block.encrypted_data_hash_},
					{":path_id", meta.path_id()},
					{":offset", (uint64_t)offset}
			});

			offset += block.blocksize_;
		}

		log_->debug() << "Added Meta of " << crypto::Base32().to_string(meta.path_id());
	}
}

std::list<Index::SignedMeta> Index::get_Meta(std::string sql, std::map<std::string, SQLValue> values){
	std::list<SignedMeta> result_list;
	for(auto row : db_->exec(sql, values))
		result_list.push_back({row[0], row[1]});
	return result_list;
}
Index::SignedMeta Index::get_Meta(const blob& path_id){
	auto meta_list = get_Meta("SELECT meta, signature FROM files WHERE path_id=:path_id LIMIT 1", {
			{":path_id", path_id}
	});

	if(meta_list.empty()) throw no_such_meta();
	return *meta_list.begin();
}
std::list<Index::SignedMeta> Index::get_Meta(){
	return get_Meta("SELECT meta, signature FROM files");
}

std::list<Index::SignedMeta> Index::containing_block(const blob& encrypted_data_hash) {
	return get_Meta("SELECT meta, signature FROM files WHERE path_id=:path_id", {{":path_id", encrypted_data_hash}});
}

} /* namespace librevault */
