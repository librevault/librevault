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
#include <util/file_util.h>
#include "EncStorage.h"
#include "folder/fs/FSFolder.h"

namespace librevault {

EncStorage::EncStorage(FSFolder& dir, ChunkStorage& chunk_storage) : AbstractStorage(dir, chunk_storage), Loggable(dir, "EncStorage") {}

std::string EncStorage::make_chunk_ct_name(const blob& ct_hash) const noexcept {
	return std::string("chunk-") + crypto::Base32().to_string(ct_hash);
}

fs::path EncStorage::make_chunk_ct_path(const blob& ct_hash) const noexcept {
	return dir_.system_path() / make_chunk_ct_name(ct_hash);
}

bool EncStorage::have_chunk(const blob& ct_hash) const noexcept {
	return fs::exists(make_chunk_ct_path(ct_hash));
}

std::shared_ptr<blob> EncStorage::get_chunk(const blob& ct_hash) const {
	try {
		auto chunk_path = make_chunk_ct_path(ct_hash);

		uint64_t chunksize = fs::file_size(chunk_path);
		if(chunksize == static_cast<uintmax_t>(-1)) throw AbstractFolder::no_such_chunk();

		std::shared_ptr<blob> chunk = std::make_shared<blob>(chunksize);

		file_wrapper chunk_file(chunk_path, "rb");
		chunk_file.ios().exceptions(std::ios_base::failbit | std::ios_base::badbit);
		chunk_file.ios().read(reinterpret_cast<char*>(chunk->data()), chunksize);

		return chunk;
	}catch(fs::filesystem_error& e) {
		throw AbstractFolder::no_such_chunk();
	}catch(std::ios_base::failure& e) {
		throw AbstractFolder::no_such_chunk();
	}
}

void EncStorage::put_chunk(const blob& ct_hash, const blob& data) {
	auto chunk_path = make_chunk_ct_path(ct_hash);
	file_wrapper chunk_file(chunk_path, "wb");
	chunk_file.ios().exceptions(std::ios_base::failbit | std::ios_base::badbit);
	chunk_file.ios().write(reinterpret_cast<const char*>(data.data()), data.size());

	log_->debug() << log_tag() << "Encrypted block " << make_chunk_ct_name(ct_hash) << " pushed into EncStorage";
}

void EncStorage::remove_chunk(const blob& ct_hash) {
	fs::remove(make_chunk_ct_path(ct_hash));

	log_->debug() << log_tag() << "Block " << make_chunk_ct_name(ct_hash) << " removed from EncStorage";
}

} /* namespace librevault */
