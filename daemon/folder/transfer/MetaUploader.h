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
#pragma once
#include "MetaInfo.h"
#include <v2/messages.h>
#include <QBitArray>
#include <QObject>

namespace librevault {

class Peer;
class Index;
class ChunkStorage;

class MetaUploader : public QObject {
  Q_OBJECT

 public:
  MetaUploader(Index* index, ChunkStorage* chunk_storage, QObject* parent);

  Q_SLOT void broadcastMeta(const QList<Peer*>& peers, const SignedMeta& smeta);
  Q_SLOT void broadcastChunk(const QList<Peer*>& peers, const QByteArray& ct_hash);

  /* Message handlers */
  Q_SLOT void handleHandshake(Peer* peer);
  Q_SLOT void handleMetaRequest(Peer* peer, const MetaInfo::PathRevision& revision);

 private:
  Index* index_;
  ChunkStorage* chunk_storage_;

  protocol::v2::IndexUpdate makeMessage(const SignedMeta& smeta);
};

} /* namespace librevault */
