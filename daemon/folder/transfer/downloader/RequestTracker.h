/* Copyright (C) 2017 Alexander Shishenko <alex@shishenko.com>
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
#include <QByteArray>
#include <QLinkedList>
#include <QList>
#include <chrono>
#include <functional>

namespace librevault {

class Peer;

struct BlockRequest {
  QByteArray ct_hash;
  quint32 offset;
  quint32 size;
  std::chrono::steady_clock::time_point started;
  std::chrono::steady_clock::time_point expires;
  Peer* peer;
};

class RequestTracker {
 public:
  int count() const { return requests_.size(); }
  int maxRequests() const;
  int freeSlots() const { return maxRequests() - count(); }
  quint32 maxBlockSize() const;
  std::chrono::seconds maxRequestTimeout() const;

  void createRequest(const QByteArray& ct_hash, quint32 offset, quint32 size, Peer* peer);

  bool completeRequest(const QByteArray& ct_hash, quint32 offset, quint32 size, Peer* peer);
  void removeExpired();
  void peerDisconnected(Peer* peer);
  void peerChoked(Peer* peer);
  void removeExisting(const QByteArray& ct_hash);

  QList<BlockRequest> requestsForChunk(const QByteArray& ct_hash);

 private:
  QLinkedList<BlockRequest> requests_;
  bool removeRequests(std::function<bool(const BlockRequest&)> pred, const QString& reason);
  void removeRequest(QMutableLinkedListIterator<BlockRequest>& it, const QString& reason);
};

}  // namespace librevault
