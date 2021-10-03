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
#include "MetaUploader.h"

#include "folder/RemoteFolder.h"
#include "folder/chunk/ChunkStorage.h"
#include "folder/meta/MetaStorage.h"

namespace librevault {

MetaUploader::MetaUploader(MetaStorage* meta_storage, ChunkStorage* chunk_storage, QObject* parent)
    : QObject(parent), meta_storage_(meta_storage), chunk_storage_(chunk_storage) {
  LOGFUNC();
}

void MetaUploader::broadcast_meta(QList<RemoteFolder*> remotes, const Meta::PathRevision& revision,
                                  const bitfield_type& bitfield) {
  for (auto remote : remotes) remote->post_have_meta(revision, bitfield);
}

void MetaUploader::handle_handshake(RemoteFolder* remote) {
  for (auto& meta : meta_storage_->getMeta())
    remote->post_have_meta(meta.meta().path_revision(), chunk_storage_->make_bitfield(meta.meta()));
}

void MetaUploader::handle_meta_request(RemoteFolder* remote, const Meta::PathRevision& revision) {
  try {
    remote->post_meta(meta_storage_->getMeta(revision),
                      chunk_storage_->make_bitfield(meta_storage_->getMeta(revision).meta()));
  } catch (MetaStorage::MetaNotFound& e) {
    LOGW("Requested nonexistent Meta");
  }
}

}  // namespace librevault
