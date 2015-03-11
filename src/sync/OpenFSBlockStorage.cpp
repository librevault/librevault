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
}

OpenFSBlockStorage::~OpenFSBlockStorage() {
	sqlite3_close(directory_db);
}

void OpenFSBlockStorage::index_file(const boost::filesystem::path& filepath){
	const char* SQL_files = "INSERT INTO files (path, filemap, signature) VALUES (?, ?, ?);";
	const char* SQL_blocks = "INSERT INTO blocks (encrypted_hash, blocksize, iv, fileid, offset) VALUES (?, ?, ?, ?, ?);";
	sqlite3_stmt *sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	if(boost::filesystem::is_regular_file(filepath)){
		BOOST_LOG_TRIVIAL(debug) << filepath;

		std::ifstream fs(filepath.string().c_str());
		cryptodiff::FileMap filemap(encryption_key);
		filemap.create(fs);
		auto filemap_s = filemap.to_string();

		sql_err_code = sqlite3_prepare_v2(directory_db, SQL_files, -1, &sqlite_stmt, 0);
		sqlite3_bind_text(sqlite_stmt, 1, filepath.string().c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_blob(sqlite_stmt, 2, filemap_s.c_str(), filemap_s.size(), SQLITE_STATIC);
		sqlite3_bind_blob(sqlite_stmt, 3, filemap_s.c_str(), filemap_s.size(), SQLITE_STATIC);
		sql_err_code = sqlite3_step(sqlite_stmt);
		sql_err_code = sqlite3_finalize(sqlite_stmt);

		auto inserted_fileid = sqlite3_last_insert_rowid(directory_db);
		uint64_t offset = 0;

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

void OpenFSBlockStorage::init(){
	char* sql_err_text = 0;
	int sql_err_code = 0;
	sql_err_code = sqlite3_exec(directory_db, "BEGIN TRANSACTION;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	sql_err_code = sqlite3_exec(directory_db, "CREATE TABLE files (id INTEGER PRIMARY KEY, path STRING NOT NULL UNIQUE, filemap BLOB NOT NULL, signature BLOB NOT NULL)", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	sql_err_code = sqlite3_exec(directory_db, "CREATE TABLE blocks (encrypted_hash BLOB PRIMARY KEY NOT NULL, blocksize INTEGER NOT NULL, iv BLOB NOT NULL, fileid INTEGER REFERENCES files (id) NOT NULL, offset INTEGER)", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	sql_err_code = sqlite3_exec(directory_db, "COMMIT TRANSACTION;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
}

void OpenFSBlockStorage::index(){
	char* sql_err_text = 0;
	int sql_err_code = 0;
	sql_err_code = sqlite3_exec(directory_db, "BEGIN TRANSACTION;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	for(auto dir_entry_it = boost::filesystem::recursive_directory_iterator(directory_path); dir_entry_it != boost::filesystem::recursive_directory_iterator(); dir_entry_it++){
		index_file(dir_entry_it->path());
	}
	sql_err_code = sqlite3_exec(directory_db, "COMMIT TRANSACTION;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
}

cryptodiff::FileMap OpenFSBlockStorage::get_FileMap(
		const boost::filesystem::path& filepath) {
}

void OpenFSBlockStorage::put_FileMap(const boost::filesystem::path& filepath,
		const cryptodiff::FileMap& filemap, const std::string& signature) {
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
