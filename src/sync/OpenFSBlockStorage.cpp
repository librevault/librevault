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
#include "FileMeta.pb.h"
#include "OpenFSBlockStorage.h"
#include <cryptopp/osrng.h>
#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <fstream>
#include <set>

namespace librevault {

OpenFSBlockStorage::OpenFSBlockStorage(const boost::filesystem::path& dirpath, const boost::filesystem::path& dbpath, cryptodiff::key_t key) :
		directory_path(dirpath), encryption_key(key) {
	sqlite3_open(dbpath.string().c_str(), &directory_db);

	char* sql_err_text = 0;
	int sql_err_code = 0;

	sql_err_code = sqlite3_exec(directory_db,
			"PRAGMA foreign_keys = ON;" \
			"CREATE TABLE IF NOT EXISTS files (id INTEGER PRIMARY KEY, path STRING NOT NULL UNIQUE, filemap BLOB NOT NULL, mtime DATETIME NOT NULL, signature BLOB NOT NULL);" \
			"CREATE TABLE IF NOT EXISTS blocks (encrypted_hash BLOB PRIMARY KEY NOT NULL, blocksize INTEGER NOT NULL, iv BLOB NOT NULL, fileid INTEGER REFERENCES files (id) ON DELETE CASCADE NOT NULL, offset INTEGER, ready BOOLEAN DEFAULT 0);",
			0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
}

OpenFSBlockStorage::~OpenFSBlockStorage() {
	sqlite3_close(directory_db);
}

boost::optional<boost::filesystem::path> OpenFSBlockStorage::relpath(boost::filesystem::path path) const {
	boost::filesystem::path relpath;
	path = absolute(path);

	auto path_elem_it = path.begin();
	for(auto dir_elem : directory_path){
		if(dir_elem != *(path_elem_it++))
			return boost::none;
	}
	for(; path_elem_it != path.end(); path_elem_it++){
		if(*path_elem_it == "." || *path_elem_it == "..")
			return boost::none;
		relpath /= *path_elem_it;
	}
	return relpath;
}

void OpenFSBlockStorage::create_index_file(const boost::filesystem::path& filepath){
	FileMeta_s file_meta;

	// Type
	auto file_status = boost::filesystem::symlink_status(filepath);
	switch(file_status.type()){
		case boost::filesystem::regular_file:
			file_meta.set_type(FileMeta_s::FileType::FileMeta_s_FileType_FILE);
			break;
		case boost::filesystem::directory_file:
			file_meta.set_type(FileMeta_s::FileType::FileMeta_s_FileType_DIRECTORY);
			break;
		case boost::filesystem::symlink_file:
			file_meta.set_type(FileMeta_s::FileType::FileMeta_s_FileType_SYMLINK);
			file_meta.set_symlink_to(boost::filesystem::read_symlink(filepath).generic_string());
			break;
	}

	// IV
	cryptodiff::iv_t iv;
	CryptoPP::AutoSeededRandomPool rng;
	rng.GenerateBlock(iv.data(), iv.size());
	file_meta.set_iv(iv.data(), iv.size());

	// EncPath
	auto relfilepath = relpath(filepath); if(relfilepath == boost::none) throw;
	std::string portable_path = relfilepath.get().generic_string();
	auto encpath = encrypt(reinterpret_cast<const uint8_t*>(portable_path.c_str()), portable_path.size(), encryption_key, iv);
	file_meta.set_encpath(encpath.data(), encpath.size());

	// mtime
	file_meta.set_mtime(boost::filesystem::last_write_time(filepath));

	if(file_status.type() == boost::filesystem::regular_file){
		BOOST_LOG_TRIVIAL(debug) << filepath;

		boost::filesystem::ifstream fs(filepath);
		cryptodiff::FileMap filemap(encryption_key);
		filemap.create(fs);

		file_meta.mutable_filemap()->ParseFromString(filemap.to_string());	// TODO: Amazingly dirty.
	}


	// Windows attributes (I don't have Windows now to test it)
#ifdef _WIN32
	file_meta.set_windows_attrib(GetFileAttributes(filepath.native().c_str()));
#endif

	// We need a new method something like put_FileMeta
	put_EncFileMap(filepath, file_meta.filemap(), "durrr", true);	// TODO: signature, of course
}

void OpenFSBlockStorage::update_index_file(const boost::filesystem::path& filepath){
	if(boost::filesystem::is_regular_file(filepath) && !boost::filesystem::is_symlink(filepath)){
		BOOST_LOG_TRIVIAL(debug) << filepath;

		boost::filesystem::ifstream fs(filepath);

		auto old_filemap = get_FileMap(filepath);
		auto new_filemap = old_filemap.update(fs);

		put_EncFileMap(filepath, new_filemap, "durrr", true);	// TODO: signature, of course
	}else{
		// TODO: Do something useful
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
		throw;	// TODO
	}
}

cryptodiff::EncFileMap OpenFSBlockStorage::get_EncFileMap(const boost::filesystem::path& filepath){
	return get_FileMap(filepath);
}

void OpenFSBlockStorage::put_EncFileMap(const boost::filesystem::path& filepath,
		const cryptodiff::EncFileMap& filemap, const std::string& signature, boost::optional<bool> force_ready) {
	const char* SQL_files = "INSERT OR REPLACE INTO files (path, filemap, mtime, signature) VALUES (?, ?, ?, ?);";
	const char* SQL_blocks = "INSERT OR REPLACE INTO blocks (encrypted_hash, blocksize, iv, fileid, offset, ready) VALUES (?, ?, ?, ?, ?, ?);";
	sqlite3_stmt* sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	auto filemap_s = filemap.to_string();

	sql_err_code = sqlite3_prepare_v2(directory_db, SQL_files, -1, &sqlite_stmt, 0);
	sqlite3_bind_text(sqlite_stmt, 1, filepath.string().c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_blob(sqlite_stmt, 2, filemap_s.c_str(), filemap_s.size(), SQLITE_STATIC);
	sqlite3_bind_int64(sqlite_stmt, 3, boost::filesystem::last_write_time(filepath));
	sqlite3_bind_blob(sqlite_stmt, 4, signature.data(), signature.size(), SQLITE_STATIC);
	sql_err_code = sqlite3_step(sqlite_stmt);
	sql_err_code = sqlite3_finalize(sqlite_stmt);

	auto inserted_fileid = sqlite3_last_insert_rowid(directory_db);

	std::set<cryptodiff::shash_t> real_filemap_hashes;
	if(force_ready == boost::none){
		boost::filesystem::ifstream fs(filepath);
		cryptodiff::FileMap real_filemap(encryption_key);
		real_filemap.create(fs);	// Performace-eater on large datasets. But will be useful on filemaps from other node to check block presence.

		for(auto real_block : real_filemap.blocks()){
			real_filemap_hashes.insert(real_block.get_encrypted_hash());
		}
	}

	uint64_t offset = 0;

	// Iterating blocks from filemap to push necessary info into database.
	for(auto block : filemap.blocks()){
		sql_err_code = sqlite3_prepare_v2(directory_db, SQL_blocks, -1, &sqlite_stmt, 0);
		sqlite3_bind_blob(sqlite_stmt, 1, block.get_encrypted_hash().data(), block.get_encrypted_hash().size(), SQLITE_STATIC);
		sqlite3_bind_int(sqlite_stmt, 2, block.get_blocksize());
		sqlite3_bind_blob(sqlite_stmt, 3, block.get_iv().data(), block.get_iv().size(), SQLITE_STATIC);
		sqlite3_bind_int64(sqlite_stmt, 4, inserted_fileid);
		sqlite3_bind_int64(sqlite_stmt, 5, offset);
		if(force_ready == boost::none){
			sqlite3_bind_int(sqlite_stmt, 6, real_filemap_hashes.count(block.get_encrypted_hash()));
		}else{
			sqlite3_bind_int(sqlite_stmt, 6, force_ready.get());
		}
		sql_err_code = sqlite3_step(sqlite_stmt);
		sql_err_code = sqlite3_finalize(sqlite_stmt);

		offset += block.get_blocksize();
	}
}

std::vector<uint8_t> OpenFSBlockStorage::get_block(
		const std::array<uint8_t, SHASH_LENGTH>& block_hash,
		cryptodiff::Block& block_meta) {
	const char* SQL = "SELECT blocks.blocksize, blocks.iv, files.path, blocks.offset, blocks.ready FROM blocks LEFT OUTER JOIN files ON blocks.fileid = files.id WHERE encrypted_hash=?";
	sqlite3_stmt* sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	std::vector<uint8_t> result;

	sql_err_code = sqlite3_prepare_v2(directory_db, SQL, -1, &sqlite_stmt, 0);
	sqlite3_bind_blob(sqlite_stmt, 1, block_hash.data(), block_hash.size(), SQLITE_STATIC);
	if(sqlite3_step(sqlite_stmt) == SQLITE_ROW){
		auto blocksize = sqlite3_column_int(sqlite_stmt, 0);
		block_meta.set_blocksize(blocksize);
		std::string iv_s(reinterpret_cast<const char*>(sqlite3_column_blob(sqlite_stmt, 1)), sqlite3_column_bytes(sqlite_stmt, 1));
		cryptodiff::iv_t iv; std::move(iv_s.begin(), iv_s.end(), iv.begin());
		block_meta.set_iv(iv);

		boost::filesystem::path filepath(reinterpret_cast<const char*>(sqlite3_column_text(sqlite_stmt, 2)));
		uint64_t offset = sqlite3_column_int64(sqlite_stmt, 3);
		bool ready = sqlite3_column_int(sqlite_stmt, 4);

		sql_err_code = sqlite3_finalize(sqlite_stmt);

		if(ready){
			boost::filesystem::ifstream fs(filepath);
			fs.seekg(offset);
			result.resize(blocksize);
			fs.read(reinterpret_cast<char*>(result.data()), blocksize);
		}
		return result;
	}else{
		throw;	//TODO: Remove this
	}
}

void OpenFSBlockStorage::put_block(
		const std::array<uint8_t, SHASH_LENGTH>& block_hash,
		const std::vector<uint8_t>& data) {
/*	const char* SQL_exists_and_update =	"SELECT SELECT 1 FROM blocks WHERE encrypted_hash=:hash LIMIT 1;" \
										"UPDATE OR IGNORE blocks SET ready=1 WHERE encrypted_hash=:hash LIMIT 1;";
	sqlite3_stmt* sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	sql_err_code = sqlite3_prepare_v2(directory_db, SQL_exists_and_update, -1, &sqlite_stmt, 0);
	sqlite3_bind_blob(sqlite_stmt, sqlite3_bind_parameter_index(sqlite_stmt, ":hash"), block_hash.data(), block_hash.size(), SQLITE_STATIC);
	if(sqlite3_step(sqlite_stmt) == SQLITE_ROW){
		bool exists = sqlite3_column_int(sqlite_stmt, 0);
		sql_err_code = sqlite3_finalize(sqlite_stmt);
		if(!exists) throw;

		block_meta.set_blocksize(blocksize);
		std::string iv_s(reinterpret_cast<const char*>(sqlite3_column_blob(sqlite_stmt, 1)), sqlite3_column_bytes(sqlite_stmt, 1));
		cryptodiff::iv_t iv; std::move(iv_s.begin(), iv_s.end(), iv.begin());
		block_meta.set_iv(iv);

		boost::filesystem::path filepath(reinterpret_cast<const char*>(sqlite3_column_text(sqlite_stmt, 2)));
		uint64_t offset = sqlite3_column_int64(sqlite_stmt, 3);
		bool ready = sqlite3_column_int(sqlite_stmt, 4);



		if(ready){
			boost::filesystem::ifstream fs(filepath);
			fs.seekg(offset);
			result.resize(blocksize);
			fs.read(reinterpret_cast<char*>(result.data()), blocksize);
		}
		return result;
	}else{
		throw;	//TODO: Remove this
	}*/
}

} /* namespace librevault */
