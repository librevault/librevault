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
#include "EncStorage.h"

#include "../../contrib/crypto/Base32.h"
#include "../../contrib/crypto/SHA3.h"

#include <boost/filesystem/fstream.hpp>

#include "SyncFS.h"

namespace librevault {
namespace syncfs {

EncStorage::EncStorage(const fs::path& block_path) : block_path(block_path), log(spdlog::stderr_logger_mt("SyncFS.EncStorage")) {
	log->debug() << "Initializing EncStorage";
}
EncStorage::~EncStorage() {}

fs::path EncStorage::make_encblock_name(const blob& block_hash) const {
	return (std::string)crypto::Base32().to(block_hash);
}

fs::path EncStorage::make_encblock_path(const blob& block_hash) const {
	return block_path / make_encblock_name(block_hash);
}

bool EncStorage::verify_encblock(const blob& block_hash, const blob& data){
	return crypto::SHA3(28).compute(data) == crypto::BinaryArray(block_hash);
}

bool EncStorage::have_encblock(const blob& block_hash){
	return fs::exists(make_encblock_path(block_hash));
}

blob EncStorage::get_encblock(const blob& block_hash){
	if(have_encblock(block_hash)){
		auto block_path = make_encblock_path(block_hash);
		auto blocksize = fs::file_size(block_path);
		blob return_value(blocksize);

		fs::ifstream block_fstream(block_path, std::ios_base::in | std::ios_base::binary);
		block_fstream.read(reinterpret_cast<char*>(return_value.data()), blocksize);

		return return_value;
	}
	throw SyncFS::no_such_block();
}

void EncStorage::put_encblock(const blob& block_hash, const blob& data){
	auto block_path = make_encblock_path(block_hash);
	fs::ofstream block_fstream(block_path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	block_fstream.write(reinterpret_cast<const char*>(data.data()), data.size());

	log->debug() << "Encrypted block " << (std::string)crypto::Base32().to(block_hash) << " pushed into EncStorage";
}

void EncStorage::remove_encblock(const blob& block_hash){
	fs::remove(make_encblock_path(block_hash));

	log->debug() << "Block " << (std::string)crypto::Base32().to(block_hash) << " removed from EncStorage";
}

} /* namespace syncfs */
} /* namespace librevault */
