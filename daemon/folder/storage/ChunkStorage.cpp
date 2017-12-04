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
#include "config/FolderParams.h"
#include "folder/FolderGroup.h"
#include <QBitArray>

namespace librevault {

ChunkStorage::ChunkStorage(FolderGroup* fgroup, Index* index, QObject* parent) :
	QObject(parent),
	fgroup_(fgroup) {
	mem_storage = new MemoryCachedStorage(this);
	enc_storage = new EncStorage(fgroup_->params(), this);
	if(fgroup_->params().secret.canDecrypt())
		open_storage = new OpenStorage(fgroup_->params(), index, this);
};

bool ChunkStorage::haveChunk(const QByteArray& ct_hash) const noexcept {
	return mem_storage->haveChunk(ct_hash) || enc_storage->haveChunk(ct_hash) || (open_storage && open_storage->haveChunk(ct_hash));
}

QByteArray ChunkStorage::getChunk(const QByteArray& ct_hash) {
	try {                                       // Cache hit
		return mem_storage->getChunk(ct_hash);
	}catch(const NoSuchChunk& e) {                  // Cache missed
		QByteArray chunk;
		try {
			chunk = enc_storage->getChunk(ct_hash);
		}catch(const NoSuchChunk& e) {
			if(open_storage)
				chunk = open_storage->getChunk(ct_hash);
			else
				throw;
		}
		mem_storage->putChunk(ct_hash, chunk); // Put into cache
		return chunk;
	}
}

void ChunkStorage::putChunk(const QByteArray& ct_hash, QFile* chunk_f) {
	enc_storage->putChunk(ct_hash, chunk_f);
	emit chunkAdded(ct_hash);
}

QBitArray ChunkStorage::makeBitfield(const MetaInfo& meta) const noexcept {
	if(meta.kind() == meta.FILE) {
		QBitArray bitfield(meta.chunks().size());

		int bitfield_idx = 0;
		for(const auto& chunk : meta.chunks())
			bitfield[bitfield_idx++] = haveChunk(chunk.ctHash());

		return bitfield;
	}else
		return QBitArray();
}

void ChunkStorage::gcChunk(const QByteArray& ct_hash) {
  if(fgroup_->params().secret.canDecrypt())
    if(open_storage->haveChunk(ct_hash) && enc_storage->haveChunk(ct_hash))
			enc_storage->removeChunk(ct_hash);
}

} /* namespace librevault */
