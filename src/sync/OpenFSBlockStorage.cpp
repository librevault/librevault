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

OpenFSBlockStorage::OpenFSBlockStorage(const boost::filesystem::path& dirpath, const boost::filesystem::path& dbpath) : directory_path(dirpath) {
	sqlite3_open(dbpath.string().c_str(), &directory_db);
}

OpenFSBlockStorage::~OpenFSBlockStorage() {
	sqlite3_close(directory_db);
}

void OpenFSBlockStorage::init(){
	char* sql_err_text = 0;
	int sql_err_code = 0;
	sql_err_code = sqlite3_exec(directory_db, "BEGIN TRANSACTION;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	sql_err_code = sqlite3_exec(directory_db, "CREATE TABLE files (id INTEGER PRIMARY KEY, path STRING NOT NULL UNIQUE, filemap BLOB NOT NULL, signature BLOB NOT NULL)", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	sql_err_code = sqlite3_exec(directory_db, "CREATE TABLE blocks (encrypted_hash BLOB PRIMARY KEY NOT NULL, blocksize INTEGER NOT NULL, iv BLOB NOT NULL, fileid INTEGER REFERENCES files (id) NOT NULL)", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
	sql_err_code = sqlite3_exec(directory_db, "COMMIT TRANSACTION;", 0, 0, &sql_err_text);
	sqlite3_free(sql_err_text);
}

void OpenFSBlockStorage::index(){
	for(auto dir_entry_it = boost::filesystem::recursive_directory_iterator(directory_path); dir_entry_it != boost::filesystem::recursive_directory_iterator(); dir_entry_it++){
		BOOST_LOG_TRIVIAL(debug) << *dir_entry_it;
		if(!boost::filesystem::is_regular_file(dir_entry_it->path())){continue;}
		const char* key_c = "12345678901234567890123456789012";
		cryptodiff::key_t key_arr; std::copy(key_c, key_c+32, &*key_arr.begin());
		cryptodiff::FileMap filemap(key_arr);
		std::ifstream fs(dir_entry_it->path().string().c_str());
		filemap.create(fs);
	}
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
