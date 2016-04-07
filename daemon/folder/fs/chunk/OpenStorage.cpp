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
#include "OpenStorage.h"

#include "folder/fs/FSFolder.h"

namespace librevault {

OpenStorage::OpenStorage(FSFolder& dir, ChunkStorage& chunk_storage) :
	AbstractStorage(dir, chunk_storage), Loggable(dir, "OpenStorage"),
	secret_(dir_.secret()) {}

bool OpenStorage::have_chunk(const blob& ct_hash) const noexcept {
	auto sql_result = dir_.index->db().exec("SELECT assembled FROM openfs "
										   "WHERE ct_hash=:ct_hash", {
										   {":ct_hash", ct_hash}
									   });
	for(auto row : sql_result) {
		if(row[0].as_int() > 0) return true;
	}
	return false;
}

std::shared_ptr<blob> OpenStorage::get_chunk(const blob& ct_hash) const {
	log_->trace() << log_tag() << "get_chunk(" << AbstractFolder::ct_hash_readable(ct_hash) << ")";

	auto metas_containing = dir_.index->containing_chunk(ct_hash);

	for(auto smeta : metas_containing) {
		// Search for chunk offset and index
		uint64_t offset = 0;
		unsigned chunk_idx = 0;
		for(auto& chunk : smeta.meta().chunks()) {
			if(chunk.ct_hash == ct_hash) break;
			offset += chunk.size;
			chunk_idx++;
		}
		if(chunk_idx > smeta.meta().chunks().size()) continue;

		// Found chunk & offset
		auto chunk = smeta.meta().chunks().at(chunk_idx);
		blob chunk_pt = blob(chunk.size);

		fs::ifstream ifs; ifs.exceptions(std::ios::failbit | std::ios::badbit);
		try {
			ifs.open(fs::absolute(smeta.meta().path(secret_), dir_.path()), std::ios::binary);
			ifs.seekg(offset);
			ifs.read(reinterpret_cast<char*>(chunk_pt.data()), chunk.size);

			std::shared_ptr<blob> chunk_ct = std::make_shared<blob>(Meta::Chunk::encrypt(chunk_pt, secret_.get_Encryption_Key(), chunk.iv));
			// Check
			if(verify_chunk(ct_hash, *chunk_ct, smeta.meta().strong_hash_type())) return chunk_ct;
		}catch(const std::ios::failure& e){}
	}
	throw AbstractFolder::no_such_chunk();
}

} /* namespace librevault */
