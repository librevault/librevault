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
#include "MetaUploader.h"
#include "folder/storage/ChunkStorage.h"
#include "folder/storage/Index.h"
#include "p2p/Peer.h"
#include <QLoggingCategory>

namespace librevault {

Q_LOGGING_CATEGORY(log_metauploader, "folder.transfer.metauploader")

MetaUploader::MetaUploader(Index* index, ChunkStorage* chunk_storage,
                           QObject* parent)
    : QObject(parent), index_(index), chunk_storage_(chunk_storage) {}

void MetaUploader::broadcastMeta(QList<Peer*> peers,
                                 const MetaInfo::PathRevision& revision,
                                 QBitArray bitfield) {
  protocol::v2::IndexUpdate update;
  update.revision = revision;
  update.bitfield = bitfield;

  for (auto peer : peers) peer->sendIndexUpdate(update);
}

void MetaUploader::handleHandshake(Peer* remote) {
  for (auto& meta : index_->getMeta()) {
    protocol::v2::IndexUpdate update;
    update.revision = meta.metaInfo().path_revision();
    update.bitfield = chunk_storage_->makeBitfield(meta.metaInfo());
    remote->sendIndexUpdate(update);
  }
}

void MetaUploader::handleMetaRequest(Peer* peer,
                                     const MetaInfo::PathRevision& revision) {
  try {
    protocol::v2::MetaResponse response;
    response.smeta = index_->getMeta(revision);
    response.bitfield = chunk_storage_->makeBitfield(response.smeta.metaInfo());
    peer->sendMetaResponse(response);
  } catch (Index::NoSuchMeta& e) {
    qCDebug(log_metauploader) << "Requested nonexistent Meta";
  }
}

} /* namespace librevault */
