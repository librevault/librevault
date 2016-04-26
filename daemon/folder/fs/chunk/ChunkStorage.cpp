/* Copyright (C) 2016 Alexander Shishenko <GamePad64@gmail.com>
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
#include "folder/fs/FSFolder.h"
#include "Client.h"

#include "folder/fs/Index.h"
#include "MemoryCachedStorage.h"
#include "EncStorage.h"
#include "OpenStorage.h"

#include "FileAssembler.h"

namespace librevault {

ChunkStorage::ChunkStorage(FSFolder& dir, Client& client) : dir_(dir), client_(client) {
	mem_storage = std::make_unique<MemoryCachedStorage>(dir, *this);
	enc_storage = std::make_unique<EncStorage>(dir, *this);
	if(dir_.params().secret.get_type() <= Secret::Type::ReadOnly) {
		open_storage = std::make_unique<OpenStorage>(dir, *this);
		file_assembler = std::make_unique<FileAssembler>(dir, *this, client_);
	}

	dir_.index->assemble_meta_signal.connect([this](const Meta& meta){
		if(open_storage && file_assembler)
			file_assembler->queue_assemble(meta);
	});
};

ChunkStorage::~ChunkStorage() {}

bool ChunkStorage::have_chunk(const blob& ct_hash) const noexcept {
	return mem_storage->have_chunk(ct_hash) || enc_storage->have_chunk(ct_hash) || (open_storage && open_storage->have_chunk(ct_hash));
}

blob ChunkStorage::get_chunk(const blob& ct_hash) {
	try {
		// Cache hit
		return *mem_storage->get_chunk(ct_hash);
	}catch(AbstractFolder::no_such_chunk& e) {
		// Cache missed
		std::shared_ptr<blob> block_ptr;
		try {
			block_ptr = enc_storage->get_chunk(ct_hash);
		}catch(AbstractFolder::no_such_chunk& e) {
			if(open_storage)
				block_ptr = open_storage->get_chunk(ct_hash);
			else
				throw;
		}
		mem_storage->put_chunk(ct_hash, block_ptr); // Put into cache
		return *block_ptr;
	}
}

void ChunkStorage::put_chunk(const blob& ct_hash, const blob& chunk) {
	enc_storage->put_chunk(ct_hash, chunk);
	if(open_storage && file_assembler)
		for(auto& smeta : dir_.index->containing_chunk(ct_hash))
			file_assembler->queue_assemble(smeta.meta());

	new_chunk_signal(ct_hash);
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
	if(open_storage)
		for(auto chunk : meta.chunks())
			if(open_storage->have_chunk(chunk.ct_hash))
				enc_storage->remove_chunk(chunk.ct_hash);
}

} /* namespace librevault */
