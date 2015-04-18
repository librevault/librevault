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
#include "FSBlockStorage.h"

namespace librevault {
namespace syncfs {

FSBlockStorage::FSBlockStorage(const fs::path& dirpath) {
	open_path = dirpath;
	system_path = open_path / ".librevault";

	directory_db = std::make_shared<SQLiteDB>(system_path / "directory.db");
	directory_db->exec("PRAGMA foreign_keys = ON;");
	directory_db->exec("CREATE TABLE IF NOT EXISTS files ("
			"id INTEGER PRIMARY KEY, "
			"path STRING UNIQUE, "
			"encpath BLOB NOT NULL UNIQUE, "
			"encpath_iv BLOB NOT NULL, "
			"mtime INTEGER NOT NULL, "
			"meta BLOB NOT NULL, "
			"signature BLOB NOT NULL);");
	directory_db->exec("CREATE TABLE IF NOT EXISTS blocks ("
			"id INTEGER PRIMARY KEY, "
			"encrypted_hash BLOB NOT NULL, "
			"blocksize INTEGER NOT NULL, "
			"iv BLOB NOT NULL, "
			"fileid INTEGER REFERENCES files (id) ON DELETE CASCADE ON UPDATE CASCADE, "
			"[offset] INTEGER, "
			"present_location INTEGER NOT NULL DEFAULT (0));");

	meta_storage = std::make_shared<MetaStorage>(directory_db);
	meta_storage = std::make_shared<Indexer>(directory_db);
	encfs_block_storage = std::make_shared<EncFSBlockStorage>(system_path);
	openfs_block_storage = std::make_shared<OpenFSBlockStorage>(directory_db, meta_storage, encfs_block_storage, system_path, open_path, aes_key);
}

FSBlockStorage::~FSBlockStorage() {}

} /* namespace syncfs */
} /* namespace librevault */
