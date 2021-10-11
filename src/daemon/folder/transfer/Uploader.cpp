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
#include "Uploader.h"

#include "folder/RemoteFolder.h"
#include "folder/chunk/ChunkStorage.h"

namespace librevault {

Uploader::Uploader(ChunkStorage* chunk_storage, QObject* parent) : QObject(parent), chunk_storage_(chunk_storage) {
  LOGFUNC();
}

void Uploader::broadcast_chunk(QList<RemoteFolder*> remotes, const QByteArray& ct_hash) {
  for (auto& remote : remotes) remote->post_have_chunk(ct_hash);
}

void Uploader::handle_interested(RemoteFolder* remote) {
  LOGFUNC();

  // TODO: write good choking algorithm.
  remote->unchoke();
}
void Uploader::handle_not_interested(RemoteFolder* remote) {
  LOGFUNC();

  // TODO: write good choking algorithm.
  remote->choke();
}

void Uploader::handle_block_request(RemoteFolder* remote, const QByteArray& ct_hash, uint32_t offset,
                                    uint32_t size) noexcept {
  try {
    if (!remote->am_choking() && remote->peer_interested())
      remote->post_block(ct_hash, offset, get_block(ct_hash, offset, size));
  } catch (ChunkStorage::ChunkNotFound& e) {
    LOGW(e.what());
  } catch (Uploader::ChunkOutOfBounds& e) {
    LOGW(e.what());
  }
}

QByteArray Uploader::get_block(const QByteArray& ct_hash, uint32_t offset, uint32_t size) {
  auto chunk = chunk_storage_->get_chunk(ct_hash);
  if (((int)offset < chunk.size()) && ((int)size <= (chunk.size() - (int)offset)))
    return chunk.mid(offset, size);
  else
    throw Uploader::ChunkOutOfBounds();
}

}  // namespace librevault
