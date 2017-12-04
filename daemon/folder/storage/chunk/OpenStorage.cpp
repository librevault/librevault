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
#include "config/models.h"
#include "folder/storage/ChunkStorage.h"
#include "folder/storage/Index.h"
#include <path_normalizer.h>
#include <QLoggingCategory>

namespace librevault {

Q_LOGGING_CATEGORY(log_openstorage, "folder.storage.chunk.openstorage")

OpenStorage::OpenStorage(const FolderSettings& params, Index* index, QObject* parent)
    : QObject(parent), params_(params), index_(index) {}

bool OpenStorage::haveChunk(const QByteArray& ct_hash) const noexcept {
  return index_->isChunkAssembled(ct_hash);
}

QByteArray OpenStorage::getChunk(const QByteArray& ct_hash) const {
  qCDebug(log_openstorage) << "get_chunk(" << ct_hash.toHex() << ")";

  for (auto& smeta : index_->containingChunk(ct_hash)) {
    // Search for chunk offset and index
    uint64_t offset = 0;
    int chunk_idx = 0;
    for (auto& chunk : smeta.metaInfo().chunks()) {
      if (chunk.ctHash() == ct_hash) break;
      offset += chunk.size();
      chunk_idx++;
    }
    if (chunk_idx > smeta.metaInfo().chunks().size()) continue;

    // Found chunk & offset
    auto chunk = smeta.metaInfo().chunks().at(chunk_idx);
    QByteArray chunk_pt(chunk.size(), 0);

    QFile f(metakit::absolutizePath(
        smeta.metaInfo().path().plaintext(params_.secret.encryptionKey()), params_.path));
    if (!f.open(QIODevice::ReadOnly)) continue;
    if (!f.seek(offset)) continue;
    if (f.read(chunk_pt.data(), chunk.size()) != (qint64)chunk.size()) continue;

    QByteArray chunk_ct = ChunkInfo::encrypt(chunk_pt, params_.secret, chunk.iv());

    // Check
    if (ChunkInfo::verifyChunk(ct_hash, chunk_ct)) return chunk_ct;
  }
  throw ChunkStorage::NoSuchChunk();
}

} /* namespace librevault */
