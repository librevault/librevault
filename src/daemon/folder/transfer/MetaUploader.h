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

#include "Meta.h"
#include "util/conv_bitfield.h"
#include "util/log.h"

namespace librevault {

class RemoteFolder;
class MetaStorage;
class ChunkStorage;

class MetaUploader : public QObject {
  Q_OBJECT
  LOG_SCOPE("MetaUploader");

 public:
  MetaUploader(MetaStorage* meta_storage, ChunkStorage* chunk_storage, QObject* parent);

  void broadcast_meta(QList<RemoteFolder*> remotes, const Meta::PathRevision& revision, const bitfield_type& bitfield);

  /* Message handlers */
  void handle_handshake(RemoteFolder* remote);
  void handle_meta_request(RemoteFolder* remote, const Meta::PathRevision& revision);

 private:
  MetaStorage* meta_storage_;
  ChunkStorage* chunk_storage_;
};

}  // namespace librevault
