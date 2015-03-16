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

namespace librevault {

EncFSBlockStorage::EncFSBlockStorage(const boost::filesystem::path& dirpath, const boost::filesystem::path& dbpath) : directory_path(dirpath) {
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

EncFSBlockStorage::~EncFSBlockStorage() {
	sqlite3_close(directory_db);
}

cryptodiff::EncFileMap EncFSBlockStorage::get_EncFileMap(const boost::filesystem::path& filepath){

}

void EncFSBlockStorage::put_EncFileMap(const boost::filesystem::path& filepath,
		const cryptodiff::EncFileMap& filemap,
		const std::string& signature,
		boost::optional<bool> force_ready = boost::none){

}

std::vector<uint8_t> EncFSBlockStorage::get_block(const std::array<uint8_t, SHASH_LENGTH>& block_hash, cryptodiff::Block& block_meta);
void EncFSBlockStorage::put_block(const std::array<uint8_t, SHASH_LENGTH>& block_hash, const std::vector<uint8_t>& data);

} /* namespace librevault */
