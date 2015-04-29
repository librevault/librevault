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
#include "FSBlockStorage.h"
#include "../../contrib/crypto/Base32.h"
#include "../../contrib/crypto/SHA3.h"
#include <boost/filesystem/fstream.hpp>

namespace librevault {
namespace syncfs {

EncFSBlockStorage::EncFSBlockStorage(FSBlockStorage* parent) : parent(parent) {}

EncFSBlockStorage::~EncFSBlockStorage() {}

fs::path EncFSBlockStorage::make_encblock_path(const crypto::BinaryArray& block_hash){
	return parent->system_path / (std::string)crypto::Base32().to(block_hash);
}

bool EncFSBlockStorage::verify_encblock(const crypto::BinaryArray& block_hash, const blob& data){
	return block_hash == crypto::SHA3(28).compute(data);
}

bool EncFSBlockStorage::have_encblock(const crypto::BinaryArray& block_hash){
	return fs::exists(make_encblock_path(block_hash));
}

blob EncFSBlockStorage::get_encblock(const crypto::BinaryArray& block_hash){
	if(have_encblock(block_hash)){
		auto block_path = make_encblock_path(block_hash);
		auto blocksize = fs::file_size(block_path);
		blob return_value(blocksize);

		fs::ifstream block_fstream(block_path, std::ios_base::in | std::ios_base::binary);
		block_fstream.read(reinterpret_cast<char*>(return_value.data()), blocksize);

		return return_value;
	}
	throw parent->NoSuchBlock;
}

void EncFSBlockStorage::put_encblock(const crypto::BinaryArray& block_hash, const blob& data){
	auto block_path = make_encblock_path(block_hash);
	fs::ofstream block_fstream(block_path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	block_fstream.write(reinterpret_cast<const char*>(data.data()), data.size());

	parent->directory_db->exec("UPDATE blocks SET in_encfs=1 WHERE encrypted_hash=:encrypted_hash;", {{":encrypted_hash", (blob)block_hash}});
}

void EncFSBlockStorage::remove_encblock(const crypto::BinaryArray& block_hash){
	fs::remove(make_encblock_path(block_hash));

	parent->directory_db->exec("UPDATE blocks SET in_encfs=0 WHERE encrypted_hash=:encrypted_hash;", {{":encrypted_hash", (blob)block_hash}});
}

} /* namespace syncfs */
} /* namespace librevault */
