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
#pragma once
#include "../../pch.h"
#include "AbstractStorage.h"
#include "../../util/Loggable.h"
#include "../../util/AvailabilityMap.h"

namespace librevault {

class Client;
class FSDirectory;
class ChunkStorage : public AbstractStorage, protected Loggable {
public:
	ChunkStorage(FSDirectory& dir, Client& client);
	virtual ~ChunkStorage();

	blob get_block(const blob& encrypted_data_hash);
	void put_chunk(const blob& encrypted_data_hash, uint32_t offset, const blob& content);

private:
	FSDirectory& dir_;
	const fs::path& block_path_;

	class Block {
	public:
		Block(uint32_t size);
		~Block();

		uint64_t size() const {return file_map_.size_original();}

		blob get_block();
		void put_chunk(uint32_t offset, const blob& content);

		AvailabilityMap::const_iterator begin() {return file_map_.begin();}
		AvailabilityMap::const_iterator end() {return file_map_.end();}

		AvailabilityMap& map() {return file_map_;}

	private:
		AvailabilityMap file_map_;
		fs::path this_block_path_;
		boost::iostreams::mapped_file mapped_file_;
	};

	std::map<blob, Block> blocks_;

	std::string log_tag() const;
};

} /* namespace librevault */
