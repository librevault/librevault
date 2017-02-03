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
#include "ChunkStorage.h"

#include "MemoryCachedStorage.h"
#include "EncStorage.h"
#include "OpenStorage.h"
#include "folder/meta/Index.h"
#include "folder/meta/MetaStorage.h"

#include "AssemblerQueue.h"

namespace librevault {

ChunkStorage::ChunkStorage(const FolderParams& params,
                           MetaStorage& meta_storage,
                           PathNormalizer& path_normalizer,
                           io_service& bulk_ios,
                           io_service& serial_ios,
                           QObject* parent) :
	QObject(parent),
	meta_storage_(meta_storage) {
	mem_storage = std::make_unique<MemoryCachedStorage>(*this);
	enc_storage = std::make_unique<EncStorage>(params, *this);
	if(params.secret.get_type() <= Secret::Type::ReadOnly) {
		open_storage = std::make_unique<OpenStorage>(params, meta_storage_, path_normalizer, *this);
		archive = std::make_unique<Archive>(params, meta_storage_, path_normalizer, serial_ios);
		file_assembler = new AssemblerQueue(params, meta_storage_,  *this, path_normalizer, *archive, this);
	}

	connect(meta_storage_.index, &Index::metaAddedExternal, file_assembler, &AssemblerQueue::addAssemble);
};

ChunkStorage::~ChunkStorage() {}

bool ChunkStorage::have_chunk(const blob& ct_hash) const noexcept {
	return mem_storage->have_chunk(ct_hash) || enc_storage->have_chunk(ct_hash) || (open_storage && open_storage->have_chunk(ct_hash));
}

blob ChunkStorage::get_chunk(const blob& ct_hash) {
	try {
		// Cache hit
		return *mem_storage->get_chunk(ct_hash);
	}catch(no_such_chunk& e) {
		// Cache missed
		std::shared_ptr<blob> block_ptr;
		try {
			block_ptr = enc_storage->get_chunk(ct_hash);
		}catch(no_such_chunk& e) {
			if(open_storage)
				block_ptr = open_storage->get_chunk(ct_hash);
			else
				throw;
		}
		mem_storage->put_chunk(ct_hash, block_ptr); // Put into cache
		return *block_ptr;
	}
}

void ChunkStorage::put_chunk(const blob& ct_hash, const boost::filesystem::path& chunk_location) {
	enc_storage->put_chunk(ct_hash, chunk_location);
	for(auto& smeta : meta_storage_.index->containing_chunk(ct_hash))
		file_assembler->addAssemble(smeta);

	emit chunkAdded(ct_hash);
}

bitfield_type ChunkStorage::make_bitfield(const Meta& meta) const noexcept {
	if(meta.meta_type() == meta.FILE) {
		bitfield_type bitfield(meta.chunks().size());

		for(unsigned int bitfield_idx = 0; bitfield_idx < meta.chunks().size(); bitfield_idx++)
			if(have_chunk(meta.chunks().at(bitfield_idx).ct_hash))
				bitfield[bitfield_idx] = true;

		return bitfield;
	}else
		return bitfield_type();
}

void ChunkStorage::cleanup(const Meta& meta) {
	for(auto chunk : meta.chunks())
		if(open_storage->have_chunk(chunk.ct_hash))
			enc_storage->remove_chunk(chunk.ct_hash);
}

} /* namespace librevault */
