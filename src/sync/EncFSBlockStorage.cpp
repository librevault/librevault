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
#include "EncFSBlockStorage.h"
#include "../util.h"

namespace librevault {

EncFSBlockStorage::EncFSBlockStorage(const boost::filesystem::path& dirpath, const boost::filesystem::path& dbpath) : directory_path(dirpath) {
	sqlite3_open(dbpath.string().c_str(), &directory_db);

	char* sql_err_text = 0;
	int sql_err_code = 0;

	sql_err_code = sqlite3_exec(directory_db,
			"PRAGMA foreign_keys = ON; "
			"CREATE TABLE IF NOT EXISTS files ("
				"id INTEGER PRIMARY KEY, "
				"path STRING NOT NULL UNIQUE, "
				"encpath BLOB NOT NULL UNIQUE, "
				"mtime INTEGER NOT NULL, "
				"meta BLOB NOT NULL, "
				"signature BLOB NOT NULL"
			"); "
			"CREATE TABLE IF NOT EXISTS blocks ("
				"id INTEGER PRIMARY KEY, "
				"encrypted_hash BLOB NOT NULL, "
				"blocksize INTEGER NOT NULL, "
				"iv BLOB NOT NULL"
			"); "
			"CREATE TABLE IF NOT EXISTS block_locations ("
				"blockid INTEGER REFERENCES blocks (id) ON DELETE CASCADE NOT NULL, "
				"fileid INTEGER REFERENCES files (id) ON DELETE CASCADE, "
				"[offset] INTEGER"
			");",
			0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
}

EncFSBlockStorage::~EncFSBlockStorage() {
	sqlite3_close(directory_db);
}

boost::filesystem::path EncFSBlockStorage::encrypted_block_path(const cryptodiff::shash_t& block_hash){
	return boost::filesystem::canonical(encblocks_path / to_base32(block_hash.data(), block_hash.size()));
}

int64_t EncFSBlockStorage::put_FileMeta(const FileMeta& meta, const std::vector<uint8_t>& signature) {
	const char* SQL_files = "INSERT OR REPLACE INTO files (encpath, mtime, meta, signature) VALUES (:encpath, :mtime, :meta, :signature);";
	const char* SQL_blocks = "INSERT OR REPLACE INTO blocks (encrypted_hash, blocksize, iv) VALUES (:encrypted_hash, :blocksize, :iv);";
	sqlite3_stmt* sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	sql_err_code = sqlite3_prepare_v2(directory_db, SQL_files, -1, &sqlite_stmt, 0);

	// EncPath
	sqlite3_bind_blob(sqlite_stmt,
			sqlite3_bind_parameter_index(sqlite_stmt, ":encpath"),
			reinterpret_cast<const char*>(meta.encpath().data()),
			meta.encpath().size(),
			SQLITE_STATIC);

	// mtime
	sqlite3_bind_int64(sqlite_stmt, sqlite3_bind_parameter_index(sqlite_stmt, ":mtime"), meta.mtime());

	// meta
	auto meta_s = meta.SerializeAsString();
	sqlite3_bind_blob(sqlite_stmt, sqlite3_bind_parameter_index(sqlite_stmt, ":meta"), meta_s.c_str(), meta_s.size(), SQLITE_STATIC);

	// signature
	sqlite3_bind_blob(sqlite_stmt, sqlite3_bind_parameter_index(sqlite_stmt, ":signature"), signature.data(), signature.size(), SQLITE_STATIC);

	sql_err_code = sqlite3_step(sqlite_stmt);
	sql_err_code = sqlite3_finalize(sqlite_stmt);

	auto inserted_fileid = sqlite3_last_insert_rowid(directory_db);

	uint64_t offset = 0;
	// Iterating blocks from filemap to push necessary info into database.
	auto encrypted_hash_index = sqlite3_bind_parameter_index(sqlite_stmt, ":encrypted_hash");
	auto blocksize_index = sqlite3_bind_parameter_index(sqlite_stmt, ":blocksize");
	auto iv_index = sqlite3_bind_parameter_index(sqlite_stmt, ":iv");
	for(auto block : meta.filemap().blocks()){
		sql_err_code = sqlite3_prepare_v2(directory_db, SQL_blocks, -1, &sqlite_stmt, 0);
		sqlite3_bind_blob(sqlite_stmt, encrypted_hash_index, block.encrypted_hash().data(), block.encrypted_hash().size(), SQLITE_STATIC);
		sqlite3_bind_int(sqlite_stmt, blocksize_index, block.blocksize());
		sqlite3_bind_blob(sqlite_stmt, iv_index, block.iv().data(), block.iv().size(), SQLITE_STATIC);

		sql_err_code = sqlite3_step(sqlite_stmt);
		sql_err_code = sqlite3_finalize(sqlite_stmt);

		offset += block.blocksize();
	}
	return inserted_fileid;
}

FileMeta EncFSBlockStorage::get_FileMeta(int64_t rowid, std::vector<uint8_t>& signature){
	const char* SQL = "SELECT meta, signature FROM files WHERE id=:id";
	sqlite3_stmt* sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	FileMeta result;

	sql_err_code = sqlite3_prepare_v2(directory_db, SQL, -1, &sqlite_stmt, 0);
	sqlite3_bind_int64(sqlite_stmt, sqlite3_bind_parameter_index(sqlite_stmt, ":id"), rowid);
	if(sqlite3_step(sqlite_stmt) == SQLITE_ROW){
		// FileMeta
		result.ParseFromArray(sqlite3_column_blob(sqlite_stmt, 0), sqlite3_column_bytes(sqlite_stmt, 0));
		// Signature
		auto signature_ptr = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(sqlite_stmt, 1));
		signature.assign(signature_ptr, signature_ptr+sqlite3_column_bytes(sqlite_stmt, 1));
	}
	return result;
}

FileMeta EncFSBlockStorage::get_FileMeta(std::vector<uint8_t> encpath, std::vector<uint8_t>& signature){
	const char* SQL = "SELECT meta, signature FROM files WHERE encpath=:encpath";
	sqlite3_stmt* sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	FileMeta result;

	sql_err_code = sqlite3_prepare_v2(directory_db, SQL, -1, &sqlite_stmt, 0);
	sqlite3_bind_blob(sqlite_stmt, sqlite3_bind_parameter_index(sqlite_stmt, ":encpath"), encpath.data(), encpath.size(), SQLITE_STATIC);
	if(sqlite3_step(sqlite_stmt) == SQLITE_ROW){
		// FileMeta
		result.ParseFromArray(sqlite3_column_blob(sqlite_stmt, 0), sqlite3_column_bytes(sqlite_stmt, 0));
		// Signature
		auto signature_ptr = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(sqlite_stmt, 1));
		signature.assign(signature_ptr, signature_ptr+sqlite3_column_bytes(sqlite_stmt, 1));
	}
	return result;
}

std::vector<uint8_t> EncFSBlockStorage::get_block_data(const cryptodiff::shash_t& block_hash){
	auto block_path = encrypted_block_path(block_hash);
	std::vector<uint8_t> return_value;
	if(boost::filesystem::exists(block_path)){
		auto blocksize = boost::filesystem::file_size(block_path);
		return_value.resize(blocksize);

		boost::filesystem::ifstream block_fstream(block_path);
		block_fstream.read(reinterpret_cast<char*>(return_value.data()), blocksize);
	}
	return return_value;
}

void EncFSBlockStorage::put_block_data(const cryptodiff::shash_t& block_hash, const std::vector<uint8_t>& data){
	auto block_path = encrypted_block_path(block_hash);
	boost::filesystem::ofstream block_fstream(block_path, std::ios_base::out | std::ios_base::trunc);
	block_fstream.write(reinterpret_cast<const char*>(data.data()), data.size());
}

} /* namespace librevault */
