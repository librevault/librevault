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
#include <fstream>
#include <set>

namespace librevault {

OpenFSBlockStorage::OpenFSBlockStorage(const boost::filesystem::path& dirpath,
		const boost::filesystem::path& dbpath,
		const cryptodiff::key_t& key) : EncFSBlockStorage(dirpath, dbpath), encryption_key(key) {
}

OpenFSBlockStorage::~OpenFSBlockStorage() {}

boost::optional<boost::filesystem::path> OpenFSBlockStorage::relpath(boost::filesystem::path path) const {
	boost::filesystem::path relpath;
	path = boost::filesystem::absolute(path);

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
	FileMeta file_meta;

	// Type
	auto file_status = boost::filesystem::symlink_status(filepath);
	switch(file_status.type()){
		case boost::filesystem::regular_file:
			file_meta.set_type(FileMeta::FileType::FileMeta_FileType_FILE);
			break;
		case boost::filesystem::directory_file:
			file_meta.set_type(FileMeta::FileType::FileMeta_FileType_DIRECTORY);
			break;
		case boost::filesystem::symlink_file:
			file_meta.set_type(FileMeta::FileType::FileMeta_FileType_SYMLINK);
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

	// FileMap
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

	put_FileMeta(file_meta, sign(file_meta));	// TODO: signature, of course
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
	sql_err_code = sqlite3_exec(directory_db, "BEGIN TRANSACTION; DELETE FROM files; DELETE FROM blocks; DELETE FROM block_locations;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	for(auto dir_entry_it = boost::filesystem::recursive_directory_iterator(directory_path); dir_entry_it != boost::filesystem::recursive_directory_iterator(); dir_entry_it++){
		create_index_file(dir_entry_it->path());
	}
	sql_err_code = sqlite3_exec(directory_db, "COMMIT TRANSACTION;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
}

int64_t OpenFSBlockStorage::put_FileMeta(const FileMeta& meta, const std::vector<uint8_t>& signature) {
	std::vector<uint8_t> old_signature;
	auto old_filemeta = get_FileMeta(std::vector<uint8_t>(meta.encpath().data(), meta.encpath().data()+meta.encpath().size()), old_signature);
	if(old_filemeta.SerializeAsString() != meta.SerializeAsString()){

	}

	auto fileid = EncFSBlockStorage::put_FileMeta(meta, signature);

	const char* SQL_files = "UPDATE files SET path=:path WHERE id=:id;";
	sqlite3_stmt* sqlite_stmt;
	char* sql_err_text = 0;
	int sql_err_code = 0;

	sql_err_code = sqlite3_prepare_v2(directory_db, SQL_files, -1, &sqlite_stmt, 0);

	// Path
	cryptodiff::iv_t iv; std::copy(meta.iv().begin(), meta.iv().end(), &*iv.begin());
	auto decrypted_path = decrypt(reinterpret_cast<const uint8_t*>(meta.encpath().data()), meta.encpath().size(), encryption_key, iv);
	sqlite3_bind_text(sqlite_stmt,
			sqlite3_bind_parameter_index(sqlite_stmt, ":path"),
			reinterpret_cast<const char*>(decrypted_path.size() == 0 ? nullptr : decrypted_path.data()),
			-1,
			SQLITE_STATIC);
	sqlite3_bind_int64(sqlite_stmt, sqlite3_bind_parameter_index(sqlite_stmt, ":id"), fileid);

	sql_err_code = sqlite3_step(sqlite_stmt);
	sql_err_code = sqlite3_finalize(sqlite_stmt);

	return fileid;
}

std::vector<uint8_t> OpenFSBlockStorage::get_block_data(const cryptodiff::shash_t& block_hash) {
	auto block_data = EncFSBlockStorage::get_block_data(block_hash);
	if(block_data.size() == 0){
		const char* SQL = "SELECT blocks.blocksize, blocks.iv, files.path, block_locations.offset FROM block_locations "
				"LEFT OUTER JOIN files ON blocks.fileid = files.id "
				"LEFT OUTER JOIN blocks ON blocks.blockid = blocks.id "
				"WHERE encrypted_hash=?";
		sqlite3_stmt* sqlite_stmt;
		char* sql_err_text = 0;
		int sql_err_code = 0;

		sql_err_code = sqlite3_prepare_v2(directory_db, SQL, -1, &sqlite_stmt, 0);
		sqlite3_bind_blob(sqlite_stmt, 1, block_hash.data(), block_hash.size(), SQLITE_STATIC);
		while(sqlite3_step(sqlite_stmt) == SQLITE_ROW){
			// Blocksize
			auto blocksize = sqlite3_column_int(sqlite_stmt, 0);

			// IV
			std::string iv_s(reinterpret_cast<const char*>(sqlite3_column_blob(sqlite_stmt, 1)), sqlite3_column_bytes(sqlite_stmt, 1));
			cryptodiff::iv_t iv; std::move(iv_s.begin(), iv_s.end(), iv.begin());

			// Path
			boost::filesystem::path filepath(reinterpret_cast<const char*>(sqlite3_column_text(sqlite_stmt, 2)));
			filepath = boost::filesystem::absolute(filepath, directory_path);

			// Offset
			uint64_t offset = sqlite3_column_int64(sqlite_stmt, 3);

			boost::filesystem::ifstream fs(filepath);
			fs.seekg(offset);
			block_data.resize(blocksize);

			fs.read(reinterpret_cast<char*>(block_data.data()), blocksize);
			if(!fs){
				block_data.resize(0); continue;
			}

			block_data = encrypt(block_data.data(), block_data.size(), encryption_key, iv);
			// Check
			if(compute_shash(block_data.data(), block_data.size()) == block_hash){
				break;
			}else{
				block_data.resize(0);
			}
		}
		sql_err_code = sqlite3_finalize(sqlite_stmt);
	}

	return block_data;
}

void OpenFSBlockStorage::put_block_data(const cryptodiff::shash_t& block_hash, const std::vector<uint8_t>& data) {
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
