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
#include "FSBlockStorage.h"

#include "Meta.pb.h"
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
	auto sql_result = parent->directory_db->exec("SELECT blocks.blocksize, blocks.iv, files.path, openfs.offset FROM blocks "
			"LEFT JOIN openfs ON blocks.id = openfs.blockid "
			"LEFT JOIN files ON openfs.fileid = files.id "
			"WHERE blocks.encrypted_hash=:encrypted_hash", {
					{":encrypted_hash", block_hash}
	});

	for(auto row : sql_result){
		uint64_t blocksize	= (int64_t)row[0];
		crypto::IV iv		= row[1];
		auto filepath		= fs::absolute(fs::path(row.at(2).as_text()), parent->open_path);
		uint64_t offset		= row[3].as_int();

		fs::ifstream ifs; ifs.exceptions(std::ios::failbit | std::ios::badbit);
		blob block(blocksize);

		try {
			ifs.open(filepath, std::ios::binary);
			ifs.seekg(offset);
			ifs.read(reinterpret_cast<char*>(block.data()), blocksize);

			blob encblock = crypto::encrypt(block.data(), block.size(), parent->get_aes_key(), iv);
			// Check
			if(parent->encfs_block_storage->verify_encblock(block_hash, block)){
				return {block, encblock};
			}
		}catch(const std::ios::failure& e){}
	}
	throw parent->NoSuchBlock;
}

blob OpenFSBlockStorage::get_encblock(const crypto::StrongHash& block_hash) {
	return get_both_blocks(block_hash).second;
}

blob OpenFSBlockStorage::get_block(const crypto::StrongHash& block_hash) {
	return get_both_blocks(block_hash).first;
}

bool OpenFSBlockStorage::can_assemble_file(const fs::path& relpath){
	auto sql_result = parent->directory_db->exec("SELECT block_presence.in_encfs "
			"FROM files LEFT JOIN openfs ON files.id = openfs.fileid LEFT JOIN block_presence ON openfs.blockid = block_presence.id "
			"WHERE files.path=:path AND block_presence.in_encfs = 0", {
					{":path", relpath.generic_string()}
	});
	return !sql_result.have_rows();
}

void OpenFSBlockStorage::assemble_file(const fs::path& relpath, bool delete_blocks){
	parent->directory_db->exec("SAVEPOINT assemble_file");
	try {
		if(!can_assemble_file(relpath)) throw parent->NotEnoughBlocks;
		auto blocks = parent->directory_db->exec("SELECT blocks.encrypted_hash, blocks.iv, blocks.[offset] "
				"FROM blocks JOIN files ON files.id=blocks.fileid"
				"WHERE files.path=:path"
				"ORDER BY blocks.[offset];", {
						{":path", relpath.generic_string()}
				});
		std::list<std::pair<crypto::StrongHash, crypto::IV>> write_blocks;
		for(auto row : blocks){
			write_blocks.push_back({row[0], row[1]});
		}

		fs::ofstream ofs(parent->system_path / "assembled.part", std::ios::out | std::ios::trunc | std::ios::binary);
		for(auto write_block : write_blocks){
			blob block = parent->get_block(write_block.first);
			ofs.write((const char*)block.data(), block.size());
		}

		ofs.close();

		auto abspath = fs::absolute(relpath, parent->open_path);
		fs::remove(abspath);
		fs::rename(parent->system_path / "assembled.part", abspath);

		parent->directory_db->exec("UPDATE openfs SET assembled=1 WHERE fileid IN (SELECT id FROM files WHERE path=:path)", {
				{":path", relpath.generic_string()}
		});

		parent->directory_db->exec("RELEASE assemble_file");

		// Delete orphan blocks
		/*if(delete_blocks){
			auto blocks_to_delete = parent->directory_db->exec("SELECT blocks.encrypted_hash FROM blocks JOIN files ON blocks.fileid = files.id "
					"WHERE blocks.encrypted_hash NOT IN (SELECT encrypted_hash FROM blocks WHERE present_location=:present_location) AND files.path=:path;", {
							{":path", relpath.generic_string()},
							{":present_location", (int64_t)parent->OPENFS}
					});
			for(auto row : blocks_to_delete){
				parent->encfs_block_storage->remove_encblock(row[0]);
			}
		}*/
	}catch(...){
		parent->directory_db->exec("ROLLBACK TO reassemble_file"); throw;
	}
}

void OpenFSBlockStorage::disassemble_file(const fs::path& relpath, bool delete_file){
	auto blocks_data = parent->directory_db->exec("SELECT blocks.encrypted_hash "
			"FROM files JOIN blocks ON files.id = blocks.fileid "
			"WHERE files.path=:path;", {
					{":path", relpath.generic_string()}
			});

	std::list<crypto::StrongHash> block_hashes_list;
	for(auto row : blocks_data){
		block_hashes_list.push_back(row[0]);
	}

	for(auto block_hash : block_hashes_list){
		parent->encfs_block_storage->put_encblock(block_hash, get_encblock(block_hash));	// TODO: We can make a single SQL query instead of making one for each block. We should do this in our optimization cycle.
	}

	/*parent->directory_db->exec("UPDATE blocks SET present_location=:present_location WHERE fileid IN (SELECT id FROM files WHERE path=:path)", {
			{":present_location", (int64_t)parent->ENCFS},
			{":path", relpath.generic_string()}
	});*/

	if(delete_file){
		fs::remove(fs::absolute(relpath, parent->open_path));
	}
}

} /* namespace syncfs */
} /* namespace librevault */
