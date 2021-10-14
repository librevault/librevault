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

#include <util/ffi.h>

#include "ChunkStorage.h"
#include "control/FolderParams.h"

namespace librevault {

EncStorage::EncStorage(const FolderParams& params, QObject* parent) : QObject(parent), inner_(bridge::encryptedstorage_new(params.system_path.toStdString())) {}

bool EncStorage::have_chunk(const QByteArray& ct_hash) const noexcept {
  QReadLocker lk(&storage_mtx_);
  return inner_->have_chunk(to_slice(ct_hash));
}

QByteArray EncStorage::get_chunk(const QByteArray& ct_hash) const {
  QReadLocker lk(&storage_mtx_);
  try {
    return from_vec(inner_->get_chunk(to_slice(ct_hash)));
  }catch (const std::exception& e) {
    throw ChunkStorage::ChunkNotFound();
  }
}

void EncStorage::put_chunk(const QByteArray& ct_hash, QFile* chunk_f) {
  QWriteLocker lk(&storage_mtx_);

  chunk_f->setParent(this);
  inner_->put_chunk(to_slice(ct_hash), to_slice(chunk_f->readAll()));
  chunk_f->deleteLater();
}

void EncStorage::remove_chunk(const QByteArray& ct_hash) {
  QWriteLocker lk(&storage_mtx_);
  inner_->remove_chunk(to_slice(ct_hash));
}

}  // namespace librevault
