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
 */
#include "EncStorage.h"

#include "ChunkStorage.h"
#include "control/FolderParams.h"
#include "crypto/Base32.h"
#include "util/readable.h"

namespace librevault {

EncStorage::EncStorage(const FolderParams& params, QObject* parent) : QObject(parent), params_(params) {}

QString EncStorage::make_chunk_ct_name(const QByteArray& ct_hash) const noexcept {
  return "chunk-" + QString::fromLatin1(ct_hash | crypto::Base32());
}

QString EncStorage::make_chunk_ct_path(const QByteArray& ct_hash) const noexcept {
  return params_.system_path + "/" + make_chunk_ct_name(ct_hash);
}

bool EncStorage::have_chunk(const QByteArray& ct_hash) const noexcept {
  QReadLocker lk(&storage_mtx_);
  return QFile::exists(make_chunk_ct_path(ct_hash));
}

QByteArray EncStorage::get_chunk(const QByteArray& ct_hash) const {
  QReadLocker lk(&storage_mtx_);

  QFile chunk_file(make_chunk_ct_path(ct_hash));
  if (!chunk_file.open(QIODevice::ReadOnly)) throw ChunkStorage::ChunkNotFound();

  return chunk_file.readAll();
}

void EncStorage::put_chunk(const QByteArray& ct_hash, QFile* chunk_f) {
  QWriteLocker lk(&storage_mtx_);

  chunk_f->setParent(this);
  chunk_f->rename(make_chunk_ct_path(ct_hash));
  chunk_f->deleteLater();

  LOGD("Encrypted block" << ct_hash_readable(ct_hash) << "pushed into EncStorage");
}

void EncStorage::remove_chunk(const QByteArray& ct_hash) {
  QWriteLocker lk(&storage_mtx_);
  QFile::remove(make_chunk_ct_path(ct_hash));

  LOGD("Block" << ct_hash_readable(ct_hash) << "removed from EncStorage");
}

}  // namespace librevault
