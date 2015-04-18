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

#include "FileMeta.pb.h"
#include <cryptopp/osrng.h>
#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>
#include <fstream>
#include <set>

namespace librevault {
namespace syncfs {

OpenFSBlockStorage::OpenFSBlockStorage(FSBlockStorage* parent) : parent(parent) {}
OpenFSBlockStorage::~OpenFSBlockStorage() {}

std::pair<blob, blob> OpenFSBlockStorage::get_both_blocks(const crypto::StrongHash& block_hash){
	auto result = parent->directory_db->exec("SELECT blocks.blocksize, blocks.iv, files.path, blocks.offset FROM blocks "
			"LEFT JOIN files ON blocks.fileid = files.id "
			"WHERE blocks.encrypted_hash=:encrypted_hash", {
					{":encrypted_hash", block_hash}
	});

	for(auto row : result){
		// Blocksize
		auto blocksize = row[0].as_int();

		// IV
		cryptodiff::IV iv = row[1].as_blob<crypto::AES_BLOCKSIZE>();

		// Path
		auto filepath = fs::absolute(fs::path(row.at(2).as_text()), parent->open_path);

		// Offset
		uint64_t offset = row[3].as_int();

		fs::ifstream ifs; ifs.exceptions(std::ios::failbit | std::ios::badbit);
		blob block(blocksize);

		try {
			ifs.open(filepath, std::ios::binary);
			ifs.seekg(offset);
			ifs.read(reinterpret_cast<char*>(block.data()), blocksize);

			blob encblock = crypto::encrypt(block.data(), block.size(), parent->get_aes_key(), iv);
			// Check
			if(crypto::compute_shash(block.data(), block.size()) == block_hash){
				return {block, encblock};
			}
		}catch(const std::ios::failure& e){}
	}
	throw parent->NoSuchBlock;
}

blob OpenFSBlockStorage::get_encblock(const cryptodiff::StrongHash& block_hash) {
	return get_both_blocks(block_hash).second;
}

blob OpenFSBlockStorage::get_block(const cryptodiff::StrongHash& block_hash) {
	return get_both_blocks(block_hash).first;
}

bool OpenFSBlockStorage::can_assemble_file(const fs::path& relpath){
	auto blocks_data = parent->directory_db->exec("SELECT blocks.encrypted_hash, files.path "
			"FROM files "
			"JOIN blocks ON files.id = blocks.fileid "
			"WHERE files.encpath=:encpath;", {
					{":path", relpath.generic_string()}
			});

	for(auto block : blocks_data){
		if(!parent->encfs_block_storage->have_encblock(block[0].as_blob<crypto::SHASH_LENGTH>())
				&& !fs::exists(fs::absolute(block[1].as_text(), parent->open_path))) return false;
	}
	return true;
}

void OpenFSBlockStorage::assemble_file(const fs::path& relpath, bool delete_blocks){
	parent->directory_db->exec("SAVEPOINT reassemble_file");
	try {
		if(!can_assemble_file(relpath)) throw parent->NotEnoughBlocks;
		auto blocks = parent->directory_db->exec("SELECT blocks.encrypted_hash, blocks.iv, blocks.[offset] "
				"FROM files "
				"WHERE files.path=:path"
				"ORDER BY blocks.[offset];", {
						{":path", relpath.generic_string()}
				});
		std::list<std::pair<crypto::StrongHash, crypto::IV>> write_blocks;
		for(auto row : blocks){
			write_blocks.push_back({row[0], row[1]});
		}

		fs::ofstream ofs(parent->system_path / "assembled.part", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
		for(auto write_block : write_blocks){
			blob block;
			try {
				block = get_block(write_block.first);
			}catch(...){
				block = parent->encfs_block_storage->get_encblock(write_block.first);
				block = crypto::decrypt(block.data(), block.size(), parent->get_aes_key(), write_block.second);
			}
			ofs.write((const char*)block.data(), block.size());
		}

		ofs.close();

		auto abspath = fs::absolute(relpath, parent->open_path);
		fs::remove(abspath);
		fs::rename(parent->system_path / "assembled.part", abspath);

		parent->directory_db->exec("RELEASE reassemble_file");

		// Delete orphan blocks
		if(delete_blocks){
			auto blocks_to_delete = parent->directory_db->exec("SELECT blocks.encrypted_hash, MIN(blocks.assembled) "
					"FROM ("
						"SELECT blocks.encrypted_hash AS file_block_hash "
						"FROM blocks "
						"JOIN files ON blocks.fileid = files.id "
						"WHERE files.path=:path"
					") "
					"JOIN blocks ON blocks.encrypted_hash = file_block_hash "
					"GROUP BY blocks.encrypted_hash "
					"HAVING MIN(blocks.assembled) = 1;", {
							{":path", relpath.generic_string()}
					});
			for(auto row : blocks_to_delete){
				parent->encfs_block_storage->remove_encblock(row[0]);
			}
		}
	}catch(...){
		parent->directory_db->exec("ROLLBACK TO reassemble_file"); throw;
	}
}

void OpenFSBlockStorage::disassemble_file(const fs::path& relpath, bool delete_file){
	auto blocks_data = parent->directory_db->exec("SELECT blocks.encrypted_hash "
			"FROM files "
			"JOIN blocks ON files.id = blocks.fileid "
			"WHERE files.path=:path;", {
					{":path", relpath.generic_string()}
			});

	std::list<crypto::StrongHash> block_hashes_list;
	for(auto row : blocks_data){
		block_hashes_list.push_back(row[0]);
	}

	for(auto block_hash : block_hashes_list){
		parent->encfs_block_storage->put_encblock(block_hash, get_encblock(block_hash));// TODO: We can make a single SQL query instead of making one for each block. We should do this in our optimization cycle.
	}

	if(delete_file){
		fs::remove(fs::absolute(relpath, parent->open_path));
	}
}

} /* namespace syncfs */
} /* namespace librevault */
