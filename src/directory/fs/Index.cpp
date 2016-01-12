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
#include "Index.h"
#include "FSDirectory.h"

namespace librevault {

Index::Index(FSDirectory& dir) : Loggable(dir, "Index"), dir_(dir) {
	bool db_path_created = fs::create_directories(dir_.db_path().parent_path());
	log_->debug() << log_tag() << "Database directory: " << dir_.db_path().parent_path() << (db_path_created ? " created" : "");

	if(fs::exists(dir_.db_path()))
		log_->debug() << log_tag() << "Opening SQLite3 DB: " << dir_.db_path();
	else
		log_->debug() << log_tag() << "Creating new SQLite3 DB: " << dir_.db_path();
	db_ = std::make_unique<SQLiteDB>(dir_.db_path());
	db_->exec("PRAGMA foreign_keys = ON;");

	db_->exec("CREATE TABLE IF NOT EXISTS files (path_id BLOB PRIMARY KEY NOT NULL, meta BLOB NOT NULL, signature BLOB NOT NULL);");
	db_->exec("CREATE TABLE IF NOT EXISTS blocks (encrypted_data_hash BLOB NOT NULL PRIMARY KEY, blocksize INTEGER NOT NULL, iv BLOB NOT NULL);");
	db_->exec("CREATE TABLE IF NOT EXISTS openfs (encrypted_data_hash BLOB NOT NULL REFERENCES blocks (encrypted_data_hash) ON DELETE CASCADE ON UPDATE CASCADE, path_id BLOB NOT NULL REFERENCES files (path_id) ON DELETE CASCADE ON UPDATE CASCADE, [offset] INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");

	db_->exec("CREATE TRIGGER IF NOT EXISTS block_deleter DELETE ON openfs BEGIN DELETE FROM blocks WHERE encrypted_data_hash NOT IN (SELECT encrypted_data_hash FROM openfs); END;");
}

/* Meta manipulators */

void Index::put_Meta(const Meta::SignedMeta& signed_meta, bool fully_assembled) {
	SQLiteSavepoint raii_transaction(*db_, "put_Meta");

	db_->exec("INSERT OR REPLACE INTO files (path_id, meta, signature) VALUES (:path_id, :meta, :signature);", {
			{":path_id", signed_meta.meta().path_id()},
			{":meta", signed_meta.raw_meta()},
			{":signature", signed_meta.signature()}
	});

	uint64_t offset = 0;
	for(auto block : signed_meta.meta().blocks()){
		db_->exec("INSERT OR IGNORE INTO blocks (encrypted_data_hash, blocksize, iv) VALUES (:encrypted_data_hash, :blocksize, :iv);", {
				{":encrypted_data_hash", block.encrypted_data_hash_},
				{":blocksize", (uint64_t)block.blocksize_},
				{":iv", block.iv_}
		});

		db_->exec("INSERT OR REPLACE INTO openfs (encrypted_data_hash, path_id, [offset], assembled) VALUES (:encrypted_data_hash, :path_id, :offset, :assembled);", {
				{":encrypted_data_hash", block.encrypted_data_hash_},
				{":path_id", signed_meta.meta().path_id()},
				{":offset", (uint64_t)offset},
				{":assembled", (uint64_t)fully_assembled}
		});

		offset += block.blocksize_;
	}

	if(fully_assembled)
		log_->debug() << log_tag() << "Added fully assembled Meta of " << dir_.path_id_readable(signed_meta.meta().path_id());
	else
		log_->debug() << log_tag() << "Added Meta of " << dir_.path_id_readable(signed_meta.meta().path_id());
}

std::list<Meta::SignedMeta> Index::get_Meta(std::string sql, std::map<std::string, SQLValue> values){
	std::list<Meta::SignedMeta> result_list;
	for(auto row : db_->exec(sql, values))
		result_list.push_back(Meta::SignedMeta(row[0], row[1], dir_.secret()));
	return result_list;
}
Meta::SignedMeta Index::get_Meta(const blob& path_id){
	auto meta_list = get_Meta("SELECT meta, signature FROM files WHERE path_id=:path_id LIMIT 1", {
			{":path_id", path_id}
	});

	if(meta_list.empty()) throw AbstractDirectory::no_such_meta();
	return *meta_list.begin();
}
std::list<Meta::SignedMeta> Index::get_Meta(){
	return get_Meta("SELECT meta, signature FROM files");
}

/* Block getter */

uint32_t Index::get_blocksize(const blob& encrypted_data_hash) {
	auto sql_result = db_->exec("SELECT blocksize FROM blocks WHERE encrypted_data_hash=:encrypted_data_hash", {
		                                   {":encrypted_data_hash", encrypted_data_hash}
	                                   });

	if(sql_result.have_rows())
		return (uint32_t)sql_result.begin()->at(0).as_uint();
	return 0;
}

std::list<Meta::SignedMeta> Index::containing_block(const blob& encrypted_data_hash) {
	return get_Meta("SELECT files.meta, files.signature FROM files JOIN openfs ON files.path_id=openfs.path_id WHERE openfs.encrypted_data_hash=:encrypted_data_hash", {{":encrypted_data_hash", encrypted_data_hash}});
}

} /* namespace librevault */
