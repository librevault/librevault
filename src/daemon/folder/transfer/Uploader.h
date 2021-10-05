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
#pragma once
#include <QObject>
#include <set>

#include "util/blob.h"
#include "util/log.h"

namespace librevault {

class RemoteFolder;
class ChunkStorage;

class Uploader : public QObject {
  Q_OBJECT
  LOG_SCOPE("Uploader");

 public:
  struct ChunkOutOfBounds : public std::runtime_error {
    ChunkOutOfBounds() : std::runtime_error("Requested block is out of chunk bounds") {}
  };

  Uploader(ChunkStorage* chunk_storage, QObject* parent);

  void broadcast_chunk(QList<RemoteFolder*> remotes, const blob& ct_hash);

  /* Message handlers */
  void handle_interested(RemoteFolder* remote);
  void handle_not_interested(RemoteFolder* remote);

  void handle_block_request(RemoteFolder* remote, const blob& ct_hash, uint32_t offset, uint32_t size) noexcept;

 private:
  ChunkStorage* chunk_storage_;

  QByteArray get_block(const blob& ct_hash, uint32_t offset, uint32_t size);
};

}  // namespace librevault
