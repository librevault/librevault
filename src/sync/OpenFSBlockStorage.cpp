/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "OpenFSBlockStorage.h"
#include <boost/log/trivial.hpp>
#include <fstream>

namespace librevault {

OpenFSBlockStorage::OpenFSBlockStorage(const boost::filesystem::path& dirpath, const boost::filesystem::path& dbpath, cryptodiff::key_t key) :
		directory_path(dirpath), encryption_key(key) {
	sqlite3_open(dbpath.string().c_str(), &directory_db);

	char* sql_err_text = 0;
	int sql_err_code = 0;
	sql_err_code = sqlite3_exec(directory_db, "PRAGMA foreign_keys = ON;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);

	sql_err_code = sqlite3_exec(directory_db, "BEGIN TRANSACTION;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	sql_err_code = sqlite3_exec(directory_db, "CREATE TABLE IF NOT EXISTS files (id INTEGER PRIMARY KEY, path STRING NOT NULL UNIQUE, filemap BLOB NOT NULL, signature BLOB NOT NULL)", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	sql_err_code = sqlite3_exec(directory_db, "CREATE TABLE IF NOT EXISTS blocks (encrypted_hash BLOB PRIMARY KEY NOT NULL, blocksize INTEGER NOT NULL, iv BLOB NOT NULL, fileid INTEGER REFERENCES files (id) ON DELETE CASCADE NOT NULL, offset INTEGER, ready BOOLEAN DEFAULT FALSE)", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	sql_err_code = sqlite3_exec(directory_db, "COMMIT TRANSACTION;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
}

OpenFSBlockStorage::~OpenFSBlockStorage() {
	sqlite3_close(directory_db);
}

void OpenFSBlockStorage::create_index_file(const boost::filesystem::path& filepath){
	const char* SQL_files = "INSERT INTO files (path, filemap, signature) VALUES (?, ?, ?);";
	const char* SQL_blocks = "INSERT INTO blocks (encrypted_hash, blocksize, iv, fileid, offset, ready) VALUES (?, ?, ?, ?, ?, true);";
	sqlite3_stmt* sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	if(boost::filesystem::is_regular_file(filepath)){
		BOOST_LOG_TRIVIAL(debug) << filepath;

		std::ifstream fs(filepath.string().c_str());
		cryptodiff::FileMap filemap(encryption_key);
		filemap.create(fs);

		put_FileMap(filepath, filemap, "durrr");

		auto inserted_fileid = sqlite3_last_insert_rowid(directory_db);
		uint64_t offset = 0;

		// Iterating blocks from filemap to push necessary info into database.
		for(auto block : filemap.blocks()){
			sql_err_code = sqlite3_prepare_v2(directory_db, SQL_blocks, -1, &sqlite_stmt, 0);
			sqlite3_bind_blob(sqlite_stmt, 1, block.get_encrypted_hash().data(), block.get_encrypted_hash().size(), SQLITE_STATIC);
			sqlite3_bind_int(sqlite_stmt, 2, block.get_blocksize());
			sqlite3_bind_blob(sqlite_stmt, 3, block.get_iv().data(), block.get_iv().size(), SQLITE_STATIC);
			sqlite3_bind_int64(sqlite_stmt, 4, inserted_fileid);
			sqlite3_bind_int64(sqlite_stmt, 5, offset);
			sql_err_code = sqlite3_step(sqlite_stmt);
			sql_err_code = sqlite3_finalize(sqlite_stmt);

			offset += block.get_blocksize();
		}
	}
}

void OpenFSBlockStorage::update_index_file(const boost::filesystem::path& filepath){
	const char* SQL_files = "INSERT INTO files (path, filemap, signature) VALUES (?, ?, ?);";
	const char* SQL_blocks = "INSERT INTO blocks (encrypted_hash, blocksize, iv, fileid, offset, ready) VALUES (?, ?, ?, ?, ?, true);";
	sqlite3_stmt* sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	if(boost::filesystem::is_regular_file(filepath)){
		BOOST_LOG_TRIVIAL(debug) << filepath;

		std::ifstream fs(filepath.string().c_str());
		cryptodiff::FileMap filemap(encryption_key);
		filemap.create(fs);

		put_FileMap(filepath, filemap, "durrr");

		auto inserted_fileid = sqlite3_last_insert_rowid(directory_db);
		uint64_t offset = 0;

		// Iterating blocks from filemap to push necessary info into database.
		for(auto block : filemap.blocks()){
			sql_err_code = sqlite3_prepare_v2(directory_db, SQL_blocks, -1, &sqlite_stmt, 0);
			sqlite3_bind_blob(sqlite_stmt, 1, block.get_encrypted_hash().data(), block.get_encrypted_hash().size(), SQLITE_STATIC);
			sqlite3_bind_int(sqlite_stmt, 2, block.get_blocksize());
			sqlite3_bind_blob(sqlite_stmt, 3, block.get_iv().data(), block.get_iv().size(), SQLITE_STATIC);
			sqlite3_bind_int64(sqlite_stmt, 4, inserted_fileid);
			sqlite3_bind_int64(sqlite_stmt, 5, offset);
			sql_err_code = sqlite3_step(sqlite_stmt);
			sql_err_code = sqlite3_finalize(sqlite_stmt);

			offset += block.get_blocksize();
		}
	}
}

void OpenFSBlockStorage::create_index(){
	char* sql_err_text = 0;
	int sql_err_code = 0;
	sql_err_code = sqlite3_exec(directory_db, "BEGIN TRANSACTION; DELETE FROM files; DELETE FROM blocks;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	for(auto dir_entry_it = boost::filesystem::recursive_directory_iterator(directory_path); dir_entry_it != boost::filesystem::recursive_directory_iterator(); dir_entry_it++){
		create_index_file(dir_entry_it->path());
	}
	sql_err_code = sqlite3_exec(directory_db, "COMMIT TRANSACTION;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
}

cryptodiff::FileMap OpenFSBlockStorage::get_FileMap(
		const boost::filesystem::path& filepath) {
	const char* SQL = "SELECT filemap FROM files WHERE path=?;";

	sqlite3_stmt* sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	sql_err_code = sqlite3_prepare_v2(directory_db, SQL, -1, &sqlite_stmt, 0);
	sqlite3_bind_text(sqlite_stmt, 1, filepath.string().c_str(), -1, SQLITE_STATIC);
	if(sqlite3_step(sqlite_stmt) == SQLITE_ROW){
		cryptodiff::FileMap filemap(encryption_key);
		filemap.from_array(reinterpret_cast<const uint8_t*>(sqlite3_column_blob(sqlite_stmt, 0)), sqlite3_column_bytes(sqlite_stmt, 0));
		sql_err_code = sqlite3_finalize(sqlite_stmt);
		return filemap;
	}else{
		throw;
	}
}

void OpenFSBlockStorage::put_FileMap(const boost::filesystem::path& filepath,
		const cryptodiff::FileMap& filemap, const std::string& signature, boost::optional<bool> force_ready = boost::optional()) {
	const char* SQL = "INSERT OR REPLACE INTO files (path, filemap, signature) VALUES (?, ?, ?);";
	sqlite3_stmt* sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	auto filemap_s = filemap.to_string();

	sql_err_code = sqlite3_prepare_v2(directory_db, SQL, -1, &sqlite_stmt, 0);
	sqlite3_bind_text(sqlite_stmt, 1, filepath.string().c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_blob(sqlite_stmt, 2, filemap_s.c_str(), filemap_s.size(), SQLITE_STATIC);
	sqlite3_bind_blob(sqlite_stmt, 3, signature.data(), signature.size(), SQLITE_STATIC);
	sql_err_code = sqlite3_step(sqlite_stmt);
	sql_err_code = sqlite3_finalize(sqlite_stmt);
}

std::vector<uint8_t> OpenFSBlockStorage::get_block(
		const std::array<uint8_t, SHASH_LENGTH>& block_hash,
		cryptodiff::Block& block_meta) {

}

void OpenFSBlockStorage::put_block(
		const std::array<uint8_t, SHASH_LENGTH>& block_hash,
		const std::vector<uint8_t>& data) {
}

} /* namespace librevault */
