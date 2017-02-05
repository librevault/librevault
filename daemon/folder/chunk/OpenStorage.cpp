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
#include "OpenStorage.h"
#include "control/FolderParams.h"
#include "folder/chunk/ChunkStorage.h"
#include "folder/meta/Index.h"
#include "folder/meta/MetaStorage.h"
#include "folder/PathNormalizer.h"
#include "util/file_util.h"
#include "util/log.h"
#include "util/readable.h"

namespace librevault {

OpenStorage::OpenStorage(const FolderParams& params, MetaStorage* meta_storage, PathNormalizer* path_normalizer, QObject* parent) :
	QObject(parent),
	params_(params),
	meta_storage_(meta_storage),
	path_normalizer_(path_normalizer) {}

bool OpenStorage::have_chunk(const blob& ct_hash) const noexcept {
	auto sql_result = meta_storage_->index->db().exec("SELECT assembled FROM openfs WHERE ct_hash=:ct_hash AND openfs.assembled=1 LIMIT 1", {
			{":ct_hash", ct_hash}
	});
	return sql_result.have_rows();
}

std::shared_ptr<blob> OpenStorage::get_chunk(const blob& ct_hash) const {
	LOGD("get_chunk(" << ct_hash_readable(ct_hash) << ")");

	auto metas_containing = meta_storage_->index->containing_chunk(ct_hash);

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

		file_wrapper f(path_normalizer_->absolute_path(smeta.meta().path(params_.secret)), "rb");
		f.ios().exceptions(std::ios::failbit | std::ios::badbit);
		try {
			f.ios().seekg(offset);
			f.ios().read(reinterpret_cast<char*>(chunk_pt.data()), chunk.size);

			std::shared_ptr<blob> chunk_ct = std::make_shared<blob>(Meta::Chunk::encrypt(chunk_pt, params_.secret.get_Encryption_Key(), chunk.iv));
			// Check
			if(verify_chunk(ct_hash, *chunk_ct, smeta.meta().strong_hash_type())) return chunk_ct;
		}catch(const std::ios::failure& e){}
	}
	throw ChunkStorage::no_such_chunk();
}

} /* namespace librevault */
