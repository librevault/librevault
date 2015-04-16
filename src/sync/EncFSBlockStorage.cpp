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
#include <boost/filesystem/fstream.hpp>

namespace librevault {

EncFSBlockStorage::EncFSBlockStorage(const boost::filesystem::path& dirpath, const boost::filesystem::path& dbpath) : directory_path(dirpath) {
	directory_db.open(dbpath.string().c_str());
	directory_db.exec("PRAGMA foreign_keys = ON; ");
	directory_db.exec("CREATE TABLE IF NOT EXISTS files ("
				"id INTEGER PRIMARY KEY, "
				"path STRING UNIQUE, "
				"encpath BLOB NOT NULL UNIQUE, "
				"mtime INTEGER NOT NULL, "
				"meta BLOB NOT NULL, "
				"signature BLOB NOT NULL"
			");");
	directory_db.exec("CREATE TABLE IF NOT EXISTS blocks ("
				"id INTEGER PRIMARY KEY, "
				"encrypted_hash BLOB NOT NULL, "
				"blocksize INTEGER NOT NULL, "
				"iv BLOB NOT NULL"
			");");
	directory_db.exec("CREATE TABLE IF NOT EXISTS block_locations ("
				"blockid INTEGER REFERENCES blocks (id) ON DELETE CASCADE NOT NULL, "
				"fileid INTEGER REFERENCES files (id) ON DELETE CASCADE, "
				"[offset] INTEGER"
			");");
	directory_db_handle = directory_db.sqlite3_handle();
}

EncFSBlockStorage::~EncFSBlockStorage() {}

boost::filesystem::path EncFSBlockStorage::encrypted_block_path(const cryptodiff::StrongHash& block_hash){
	return boost::filesystem::canonical(encblocks_path / to_base32(block_hash.data(), block_hash.size()));
}

bool EncFSBlockStorage::block_exists(const cryptodiff::StrongHash& block_hash){
	return boost::filesystem::exists(encrypted_block_path(block_hash));
}

int64_t EncFSBlockStorage::put_FileMeta(const FileMeta& meta, const std::vector<uint8_t>& signature, bool post_locations) {
	// FileMeta
	std::vector<uint8_t> filemeta_blob(meta.ByteSize()); meta.SerializeToArray(filemeta_blob.data(), filemeta_blob.size());

	// Signature
	directory_db.exec("INSERT OR REPLACE INTO files (encpath, mtime, meta, signature) VALUES (:encpath, :mtime, :meta, :signature);", {
			{":encpath", SQLValue((const uint8_t*)meta.encpath().data(), meta.encpath().size())},
			{":mtime", SQLValue(meta.mtime())},
			{":meta", SQLValue(filemeta_blob)},
			{":signature", SQLValue(signature)}
	});

	auto inserted_fileid = directory_db.last_insert_rowid();
	uint64_t offset = 0;

	for(auto block : meta.filemap().blocks()){
		directory_db.exec("INSERT OR REPLACE INTO blocks (encrypted_hash, blocksize, iv, fileid) VALUES (:encrypted_hash, :blocksize, :iv, :fileid);", {
				{":encrypted_hash", SQLValue((const uint8_t*)block.encrypted_hash().data(), block.encrypted_hash().size())},
				{":blocksize", SQLValue((int64_t)block.blocksize())},
				{":iv", SQLValue((const uint8_t*)block.iv().data(), block.iv().size())},
				{":fileid", SQLValue(inserted_fileid)}
		});

		if(post_locations){
			directory_db.exec("INSERT OR REPLACE INTO block_locations (blockid, offset) VALUES (last_insert_rowid(), :offset);", {
					{":offset", SQLValue((int64_t)offset)}
			});

			offset += block.blocksize();
		}
	}
	return inserted_fileid;
}

FileMeta EncFSBlockStorage::get_FileMeta(int64_t rowid, std::vector<uint8_t>& signature){
	auto query_result = directory_db.exec("SELECT meta, signature FROM files WHERE id=:id;", {
			{":id", SQLValue(rowid)},
	});

	FileMeta meta;
	if(query_result.have_rows()){
		meta.ParseFromString((*query_result.begin())[0].as_text());
		signature = ((*query_result.begin())[1].as_blob());
	}

	return meta;
}

FileMeta EncFSBlockStorage::get_FileMeta(std::vector<uint8_t> encpath, std::vector<uint8_t>& signature){
	auto query_result = directory_db.exec("SELECT meta, signature FROM files WHERE encpath=:encpath;", {
			{":encpath", SQLValue(encpath)},
	});

	FileMeta meta;
	if(query_result.have_rows()){
		meta.ParseFromString((*query_result.begin())[0].as_text());
		signature = ((*query_result.begin())[1].as_blob());
	}

	return meta;
}

std::vector<uint8_t> EncFSBlockStorage::get_block_data(const cryptodiff::StrongHash& block_hash){
	std::vector<uint8_t> return_value;
	if(block_exists(block_hash)){
		auto block_path = encrypted_block_path(block_hash);
		auto blocksize = boost::filesystem::file_size(block_path);
		return_value.resize(blocksize);

		boost::filesystem::ifstream block_fstream(block_path, std::ios_base::in | std::ios_base::binary);
		block_fstream.read(reinterpret_cast<char*>(return_value.data()), blocksize);
	}
	return return_value;
}

void EncFSBlockStorage::put_block_data(const cryptodiff::StrongHash& block_hash, const std::vector<uint8_t>& data){
	auto block_path = encrypted_block_path(block_hash);
	boost::filesystem::ofstream block_fstream(block_path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	block_fstream.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void EncFSBlockStorage::remove_block_data(const cryptodiff::StrongHash& block_hash){
	boost::filesystem::remove(encrypted_block_path(block_hash));
}

} /* namespace librevault */
