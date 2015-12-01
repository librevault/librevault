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
#include "ChunkStorage.h"
#include "FSDirectory.h"
#include "../../Client.h"

namespace librevault {

// ChunkStorage
ChunkStorage::ChunkStorage(FSDirectory& dir, Client& client) : Loggable(client), dir_(dir), block_path_(dir.block_path()) {
	bool block_path_created = fs::create_directories(block_path_);
	log_->debug() << dir_.log_tag() << "Block directory: " << block_path_ << (block_path_created ? " created" : "");
}
ChunkStorage::~ChunkStorage() {}

blob ChunkStorage::get_block(const blob& encrypted_data_hash) {
	auto it = blocks_.find(encrypted_data_hash);
	if(it != blocks_.end()) {
		return it->second.get_block();
	}else throw AbstractStorage::no_such_block();
}

void ChunkStorage::put_chunk(const blob& encrypted_data_hash, uint32_t offset, const blob& content) {
	auto it = blocks_.find(encrypted_data_hash);
	if(it == blocks_.end()) {
		auto sql_result = dir_.index->db().exec("SELECT blocks.blocksize"
				                                  "FROM blocks "
				                                 "WHERE blocks.encrypted_data_hash=:encrypted_data_hash", {{":encrypted_data_hash", encrypted_data_hash}});
		it = blocks_.emplace(encrypted_data_hash, sql_result.begin()->at(0).as_uint()).first;
	}

	it->second.put_chunk(offset, content);
	if(it->second.map().size_left() == 0) {
		// TODO: Check hash. Maybe, here?
		dir_.put_block(encrypted_data_hash, content);
	}
}

std::string ChunkStorage::log_tag() const {return dir_.log_tag();}

// ChunkStorage::Block
ChunkStorage::Block::Block(uint32_t size) : file_map_(size) {
	fs::unique_path(this_block_path_);
	{
		fs::ofstream os(this_block_path_, std::ios_base::trunc);
	}
	fs::resize_file(this_block_path_, size);
	mapped_file_.open(this_block_path_);
}

ChunkStorage::Block::~Block() {
	mapped_file_.close();
	fs::remove(this_block_path_);
}

blob ChunkStorage::Block::get_block() {
	if(file_map_.size_left() == 0) {
		blob content(size());
		std::copy(mapped_file_.data(), mapped_file_.data() + size(), content.data());
		return content;
	}else throw AbstractStorage::no_such_block();
}

void ChunkStorage::Block::put_chunk(uint32_t offset, const blob& content) {
	auto inserted = file_map_.insert({offset, content.size()}).second;
	if(inserted) std::copy(content.begin(), content.end(), mapped_file_.data()+offset);
}

} /* namespace librevault */
