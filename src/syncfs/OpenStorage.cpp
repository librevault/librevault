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
#include "Meta.pb.h"
#include "../../contrib/crypto/AES_CBC.h"
#include <cryptopp/osrng.h>
#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>
#include <fstream>
#include <set>

#include "OpenStorage.h"
#include "SyncFS.h"

namespace librevault {
namespace syncfs {

OpenStorage::OpenStorage(SyncFS* syncfs, const Key& key, std::shared_ptr<SQLiteDB> directory_db, EncStorage& enc_storage, const fs::path& open_path, const fs::path& block_path) :
		syncfs(syncfs), key(key), directory_db(directory_db), enc_storage(enc_storage), open_path(open_path), block_path(block_path) {}
OpenStorage::~OpenStorage() {}

std::string OpenStorage::get_path(const blob& path_hmac){
	for(auto row : directory_db->exec("SELECT meta FROM files WHERE path_hmac=:path_hmac", {{":path_hmac", path_hmac}})){
		std::vector<uint8_t> meta_bytes = row[0];
		Meta meta; meta.ParseFromArray(meta_bytes.data(), meta_bytes.size());
		return (std::string)crypto::AES_CBC(key.get_Encryption_Key(), meta.encpath_iv()).from(meta.encpath());
	}
	throw;
}

std::pair<blob, blob> OpenStorage::get_both_blocks(const blob& block_hash){
	auto sql_result = directory_db->exec("SELECT blocks.blocksize, blocks.iv, files.meta, openfs.[offset] FROM blocks "
			"JOIN openfs ON blocks.encrypted_hash = openfs.block_encrypted_hash "
			"JOIN files ON openfs.file_path_hmac = files.path_hmac "
			"WHERE blocks.encrypted_hash=:encrypted_hash", {
					{":encrypted_hash", block_hash}
			});

	for(auto row : sql_result){
		Meta meta; meta.ParseFromString(row[2].as_text());

		uint64_t blocksize	= row[0];
		blob iv				= row[1];
		auto filepath		= fs::absolute((std::string)crypto::AES_CBC(key.get_Encryption_Key(), meta.encpath_iv()).from(meta.encpath()), open_path);
		uint64_t offset		= row[3];

		fs::ifstream ifs; ifs.exceptions(std::ios::failbit | std::ios::badbit);
		blob block(blocksize);

		try {
			ifs.open(filepath, std::ios::binary);
			ifs.seekg(offset);
			ifs.read(reinterpret_cast<char*>(block.data()), blocksize);

			blob encblock = crypto::AES_CBC(key.get_Encryption_Key(), iv, blocksize % 16 == 0 ? false : true).encrypt(block);
			// Check
			if(enc_storage.verify_encblock(block_hash, encblock)) return {block, encblock};
		}catch(const std::ios::failure& e){}
	}
	throw SyncFS::no_such_block();
}

blob OpenStorage::get_encblock(const blob& block_hash) {
	return get_both_blocks(block_hash).second;
}

blob OpenStorage::get_block(const blob& block_hash) {
	return get_both_blocks(block_hash).first;
}

void OpenStorage::assemble(bool delete_blocks){
	auto raii_lock = SQLiteLock(directory_db.get());
	auto blocks = directory_db->exec("SELECT files.path_hmac, openfs.\"offset\", blocks.encrypted_hash, blocks.iv "
			"FROM openfs "
			"JOIN files ON openfs.file_path_hmac=files.path_hmac "
			"JOIN blocks ON openfs.block_encrypted_hash=blocks.encrypted_hash "
			"WHERE assembled=0");
	std::map<blob, std::map<uint64_t, std::pair<blob, blob>>> write_blocks;
	for(auto row : blocks){
		blob path_hmac = row[0].as_blob();
		uint64_t offset = row[1].as_uint();
		blob enchash = row[2].as_blob();
		blob enchash_iv = row[3].as_blob();
		write_blocks[path_hmac][offset] = {enchash, enchash_iv};
	}

	fs::path assembled_path = block_path / "assembled.part";
	for(auto file : write_blocks){
		fs::ofstream ofs(assembled_path, std::ios::out | std::ios::trunc | std::ios::binary);

		blob path_hmac = file.first;
		for(auto block : file.second){
			auto offset = block.first;
			auto block_hash = block.second.first;
			auto iv = block.second.second;

			blob block_data = syncfs->get_block(block_hash);
			ofs.write((const char*)block_data.data(), block_data.size());
		}
		ofs.close();

		auto abspath = fs::absolute(get_path(path_hmac), open_path);
		fs::remove(abspath);
		fs::rename(assembled_path, abspath);

		directory_db->exec("UPDATE openfs SET assembled=1 WHERE file_path_hmac=:file_path_hmac", {{":file_path_hmac", path_hmac}});
	}
}

void OpenStorage::disassemble(const std::string& file_path, bool delete_file){
	blob path_hmac = crypto::HMAC_SHA3_224(key.get_Encryption_Key()).to(file_path);

	std::list<blob> encrypted_hashes;
	auto blocks_data = directory_db->exec("SELECT block_encrypted_hash "
			"FROM openfs WHERE file_path_hmac=:file_path_hmac", {
					{":file_path_hmac", path_hmac}
			});
	for(auto row : blocks_data)
		encrypted_hashes.push_back(row[0]);
	for(auto encrypted_hash : encrypted_hashes)
		if(!enc_storage.have_encblock(encrypted_hash)) enc_storage.put_encblock(encrypted_hash, get_encblock(encrypted_hash));

	if(delete_file){
		directory_db->exec("UPDATE openfs SET assembled=0 WHERE file_path_hmac=:path_hmac", {
				{":path_hmac", path_hmac}
		});
		fs::remove(fs::absolute(file_path, open_path));
	}
}

} /* namespace syncfs */
} /* namespace librevault */
