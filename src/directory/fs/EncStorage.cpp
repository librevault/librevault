/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "EncStorage.h"
#include "FSDirectory.h"

namespace librevault {

EncStorage::EncStorage(FSDirectory& dir, Client& client) : log_(spdlog::get("Librevault")), dir_(dir), block_path_(dir.block_path()) {
	bool block_path_created = fs::create_directories(block_path_);
	log_->debug() << dir_.log_tag() << "Block directory: " << block_path_ << (block_path_created ? " created" : "");
}
EncStorage::~EncStorage() {}

fs::path EncStorage::make_encblock_name(const blob& encrypted_data_hash) const {
	return crypto::Base32().to_string(encrypted_data_hash);
}

fs::path EncStorage::make_encblock_path(const blob& encrypted_data_hash) const {
	return block_path_ / make_encblock_name(encrypted_data_hash);
}

bool EncStorage::have_block(const blob& encrypted_data_hash) {
	return fs::exists(make_encblock_path(encrypted_data_hash));
}

blob EncStorage::get_block(const blob& encrypted_data_hash) {
	auto block_path = make_encblock_path(encrypted_data_hash);

	auto blocksize = fs::file_size(block_path);
	if(blocksize == static_cast<uintmax_t>(-1)) throw no_such_block();

	blob block(blocksize);

	try {
		fs::ifstream block_fstream;
		block_fstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		block_fstream.open(block_path, std::ios_base::in | std::ios_base::binary);
		block_fstream.read(reinterpret_cast<char*>(block.data()), blocksize);
	}catch(std::ifstream::failure& e) {
		throw no_such_block();
	}

	return block;
}

void EncStorage::put_block(const blob& encrypted_data_hash, const blob& data) {
	auto block_path = make_encblock_path(encrypted_data_hash);
	fs::ofstream block_fstream(block_path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	block_fstream.write(reinterpret_cast<const char*>(data.data()), data.size());

	log_->debug() << dir_.log_tag() << "Encrypted block " << make_encblock_name(encrypted_data_hash) << " pushed into EncStorage";
}

void EncStorage::remove_block(const blob& encrypted_data_hash) {
	fs::remove(make_encblock_path(encrypted_data_hash));

	log_->debug() << dir_.log_tag() << "Block " << make_encblock_name(encrypted_data_hash) << " removed from EncStorage";
}

} /* namespace librevault */
