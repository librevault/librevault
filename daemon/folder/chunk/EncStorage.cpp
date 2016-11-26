/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "EncStorage.h"
#include "folder/AbstractFolder.h"
#include "util/fs.h"
#include "util/log.h"
#include "util/file_util.h"
#include <librevault/crypto/Base32.h>

namespace librevault {

EncStorage::EncStorage(const FolderParams& params, ChunkStorage& chunk_storage) : AbstractStorage(chunk_storage), params_(params) {}

std::string EncStorage::make_chunk_ct_name(const blob& ct_hash) const noexcept {
	return std::string("chunk-") + crypto::Base32().to_string(ct_hash);
}

fs::path EncStorage::make_chunk_ct_path(const blob& ct_hash) const noexcept {
	return params_.system_path / make_chunk_ct_name(ct_hash);
}

bool EncStorage::have_chunk(const blob& ct_hash) const noexcept {
	std::lock_guard<std::mutex> lk(storage_mtx_);
	return fs::exists(make_chunk_ct_path(ct_hash));
}

std::shared_ptr<blob> EncStorage::get_chunk(const blob& ct_hash) const {
	std::lock_guard<std::mutex> lk(storage_mtx_);
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

void EncStorage::put_chunk(const blob& ct_hash, const fs::path& chunk_location) {
	std::lock_guard<std::mutex> lk(storage_mtx_);
	file_move(chunk_location, make_chunk_ct_path(ct_hash));

	LOGD("Encrypted block " << make_chunk_ct_name(ct_hash) << " pushed into EncStorage");
}

void EncStorage::remove_chunk(const blob& ct_hash) {
	std::lock_guard<std::mutex> lk(storage_mtx_);
	fs::remove(make_chunk_ct_path(ct_hash));

	LOGD("Block " << make_chunk_ct_name(ct_hash) << " removed from EncStorage");
}

} /* namespace librevault */
