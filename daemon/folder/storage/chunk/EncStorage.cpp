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
#include "crypto/Base32.h"
#include "control/FolderSettings.h"
#include "folder/storage/ChunkStorage.h"
#include <QLoggingCategory>

namespace librevault {

Q_LOGGING_CATEGORY(log_encstorage, "folder.storage.chunk.openstorage")

EncStorage::EncStorage(const models::FolderSettings& params, QObject* parent) : QObject(parent), params_(params) {}

QString EncStorage::makeChunkCtName(const QByteArray& ct_hash) const noexcept {
  return "chunk-" + QString::fromLatin1(toBase32(ct_hash));
}

QString EncStorage::makeChunkCtPath(const QByteArray& ct_hash) const noexcept {
  return params_.effectiveSystemPath() + "/" + makeChunkCtName(ct_hash);
}

bool EncStorage::haveChunk(const QByteArray& ct_hash) const noexcept {
  QReadLocker lk(&storage_mtx_);
  return QFile::exists(makeChunkCtPath(ct_hash));
}

QByteArray EncStorage::getChunk(const QByteArray& ct_hash) const {
  QReadLocker lk(&storage_mtx_);

  QFile chunk_file(makeChunkCtPath(ct_hash));
  if (!chunk_file.open(QIODevice::ReadOnly)) throw ChunkStorage::NoSuchChunk();

  return chunk_file.readAll();
}

void EncStorage::putChunk(const QByteArray& ct_hash, QFile* chunk_f) {
  QWriteLocker lk(&storage_mtx_);

  chunk_f->setParent(this);
  chunk_f->rename(makeChunkCtPath(ct_hash));
  chunk_f->deleteLater();

  qCDebug(log_encstorage) << "Encrypted block" << ct_hash.toHex() << "pushed into EncStorage";
}

void EncStorage::removeChunk(const QByteArray& ct_hash) {
  QWriteLocker lk(&storage_mtx_);
  QFile::remove(makeChunkCtPath(ct_hash));

  qCDebug(log_encstorage) << "Block" << ct_hash.toHex() << "removed from EncStorage";
}

} /* namespace librevault */
