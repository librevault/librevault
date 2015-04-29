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
#include "../../contrib/crypto/AES_CBC.h"

namespace librevault {
namespace syncfs {

FSBlockStorage::FSBlockStorage(fs::path dirpath) : open_path(std::move(dirpath)) {
	system_dirname = ".librevault";
	system_path = open_path / system_dirname;

	fs::create_directories(open_path);
	fs::create_directories(system_path);

	init_db();

	meta_storage = std::make_shared<MetaStorage>(this);
	indexer = std::make_shared<Indexer>(this);
	encfs_block_storage = std::make_shared<EncFSBlockStorage>(this);
	openfs_block_storage = std::make_shared<OpenFSBlockStorage>(this);
}

FSBlockStorage::FSBlockStorage(fs::path dirpath, crypto::BinaryArray aes_key) : FSBlockStorage(std::move(dirpath)) {
	this->aes_key = std::move(aes_key);
}

FSBlockStorage::~FSBlockStorage() {}

void FSBlockStorage::init_db(){
	directory_db = std::make_shared<SQLiteDB>(system_path / "directory.db");
	directory_db->exec("PRAGMA foreign_keys = ON;");

	directory_db->exec("CREATE TABLE IF NOT EXISTS files (id INTEGER PRIMARY KEY, path STRING UNIQUE, encpath BLOB NOT NULL UNIQUE, encpath_iv BLOB NOT NULL, mtime INTEGER NOT NULL, meta BLOB NOT NULL, signature BLOB NOT NULL);");
	directory_db->exec("CREATE TABLE IF NOT EXISTS blocks (id INTEGER PRIMARY KEY, encrypted_hash BLOB NOT NULL UNIQUE, blocksize INTEGER NOT NULL, iv BLOB NOT NULL, in_encfs BOOLEAN NOT NULL DEFAULT (0));");
	directory_db->exec("CREATE TABLE IF NOT EXISTS openfs (blockid INTEGER REFERENCES blocks (id) ON DELETE CASCADE ON UPDATE CASCADE NOT NULL, fileid INTEGER REFERENCES files (id) ON DELETE CASCADE ON UPDATE CASCADE NOT NULL, [offset] INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");
	directory_db->exec("CREATE VIEW IF NOT EXISTS block_presence AS SELECT DISTINCT id, CASE WHEN blocks.in_encfs = 1 OR openfs.assembled = 1 THEN 1 ELSE 0 END AS present, blocks.in_encfs AS in_encfs, openfs.assembled AS in_openfs FROM blocks LEFT JOIN openfs ON blocks.id = openfs.blockid;");

	directory_db->exec("CREATE TRIGGER IF NOT EXISTS block_deleter AFTER DELETE ON files BEGIN DELETE FROM blocks WHERE id NOT IN (SELECT blockid FROM openfs); END;");
}

void FSBlockStorage::create_index(){
	indexer->create_index();
}

void FSBlockStorage::update_index(){
	indexer->update_index();
}

blob FSBlockStorage::get_block(const crypto::BinaryArray& block_hash) {
	try {
		auto encblock = encfs_block_storage->get_encblock(block_hash);
		auto sql_result = directory_db->exec("SELECT iv FROM blocks WHERE encrypted_hash=:encrypted_hash LIMIT 1;");
		auto block = crypto::AES_CBC(aes_key, sql_result.begin()[0].as_blob()).decrypt(encblock);
		return block;
	}catch(const Errors& e){
		return openfs_block_storage->get_block(block_hash);
	}
}

blob FSBlockStorage::get_encblock(const crypto::BinaryArray& block_hash) {
	try {
		return encfs_block_storage->get_encblock(block_hash);
	}catch(const Errors& e){
		return openfs_block_storage->get_encblock(block_hash);
	}
}

void FSBlockStorage::put_block(const crypto::BinaryArray& block_hash, const blob& data) {
	auto sql_result = directory_db->exec("SELECT iv FROM blocks WHERE encrypted_hash=:encrypted_hash", {
			{":encrypted_hash", (blob)block_hash}
	});
	crypto::BinaryArray iv = sql_result.begin()[0].as_blob();
	sql_result.finalize();
	auto encblock = crypto::AES_CBC(aes_key, iv).encrypt(data);

	put_encblock(block_hash, encblock);
}

void FSBlockStorage::put_encblock(const crypto::BinaryArray& block_hash, const blob& data) {
	encfs_block_storage->put_encblock(block_hash, data);
}

} /* namespace syncfs */
} /* namespace librevault */
