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
#include "p2p/Peer.h"
#include <QObject>

namespace librevault {

class Peer;
class ChunkStorage;

class Uploader : public QObject {
  Q_OBJECT

 public:
  DECLARE_EXCEPTION(ChokeMismatch, "Peer is choked or not interested");
  DECLARE_EXCEPTION(BlockOutOfBounds, "Requested offset or length is out of bounds");

  Uploader(ChunkStorage* chunk_storage, QObject* parent);

  /* Message handlers */
  void handleInterested(Peer* peer);
  void handleNotInterested(Peer* peer);

  void untrackPeer(Peer* peer);

  void handleBlockRequest(
      Peer* peer, const QByteArray& ct_hash, quint32 offset, quint32 size) noexcept;

 private:
  ChunkStorage* chunk_storage_;
  QHash<Peer*, std::shared_ptr<StateGuard>> all_interested_;

  QByteArray getBlock(const QByteArray& ct_hash, quint32 offset, quint32 size);
};

} /* namespace librevault */
