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
#include "folder/storage/chunk/MemoryCachedStorage.h"
#include "folder/storage/chunk/EncStorage.h"
#include "folder/storage/chunk/OpenStorage.h"
#include "control/FolderParams.h"
#include "folder/storage/Storage.h"
#include "folder/storage/assembler/AssemblerQueue.h"
#include <QBitArray>

namespace librevault {

ChunkStorage::ChunkStorage(const FolderParams& params, Storage* meta_storage, QObject* parent) :
	QObject(parent),
  params_(params),
	storage_(meta_storage) {
	mem_storage = new MemoryCachedStorage(this);
	enc_storage = new EncStorage(params, this);
	if(params.secret.level() <= Secret::Level::ReadOnly) {
		open_storage = new OpenStorage(params, storage_, this);
	}
};

bool ChunkStorage::have_chunk(QByteArray ct_hash) const noexcept {
	return mem_storage->have_chunk(ct_hash) || enc_storage->have_chunk(ct_hash) || (open_storage && open_storage->have_chunk(ct_hash));
}

QByteArray ChunkStorage::get_chunk(QByteArray ct_hash) {
	try {                                       // Cache hit
		return mem_storage->get_chunk(ct_hash);
	}catch(no_such_chunk& e) {                  // Cache missed
		QByteArray chunk;
		try {
			chunk = enc_storage->get_chunk(ct_hash);
		}catch(no_such_chunk& e) {
			if(open_storage)
				chunk = open_storage->get_chunk(ct_hash);
			else
				throw;
		}
		mem_storage->put_chunk(ct_hash, chunk); // Put into cache
		return chunk;
	}
}

void ChunkStorage::put_chunk(QByteArray ct_hash, QFile* chunk_f) {
	enc_storage->put_chunk(ct_hash, chunk_f);
	emit chunkAdded(ct_hash);
}

QBitArray ChunkStorage::make_bitfield(const MetaInfo& meta) const noexcept {
	if(meta.kind() == meta.FILE) {
		QBitArray bitfield(meta.chunks().size());

		int bitfield_idx = 0;
		for(const auto& chunk : meta.chunks())
			bitfield[bitfield_idx++] = have_chunk(chunk.ctHash());

		return bitfield;
	}else
		return QBitArray();
}

void ChunkStorage::pruneAssembledChunks(const MetaInfo &meta) {
	for(const auto& chunk : meta.chunks())
		rebalanceChunk(chunk.ctHash());
}

void ChunkStorage::rebalanceChunk(const QByteArray& ct_hash) {
  if(params_.secret.canDecrypt())
    if(open_storage->have_chunk(ct_hash))
      enc_storage->remove_chunk(ct_hash);
}

} /* namespace librevault */
