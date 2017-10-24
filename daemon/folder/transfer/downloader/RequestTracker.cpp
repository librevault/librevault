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
#include "RequestTracker.h"
#include "control/Config.h"
#include "p2p/Peer.h"
#include "v2/messages.h"
#include <QLoggingCategory>

namespace librevault {

Q_DECLARE_LOGGING_CATEGORY(log_downloader)

int RequestTracker::maxRequests() const {
  return Config::get()->getGlobal("p2p_download_slots").toUInt();
}

quint32 RequestTracker::maxBlockSize() const {
  return Config::get()->getGlobal("p2p_block_size").toUInt();
}

std::chrono::seconds RequestTracker::maxRequestTimeout() const {
  return std::chrono::seconds(Config::get()->getGlobal("p2p_request_timeout").toUInt());
}

void RequestTracker::createRequest(
    const QByteArray& ct_hash, quint32 offset, quint32 size, Peer* peer) {
  BlockRequest request;
  request.ct_hash = ct_hash;
  request.offset = offset;
  request.size = std::min(size, maxBlockSize());
  request.started = std::chrono::steady_clock::now();
  request.expires = std::chrono::steady_clock::now() + maxRequestTimeout();
  request.peer = peer;

  protocol::v2::Message message;
  message.header.type = protocol::v2::MessageType::BLOCKREQUEST;
  message.blockrequest.ct_hash = request.ct_hash;
  message.blockrequest.offset = request.offset;
  message.blockrequest.length = request.size;
  peer->send(message);

  requests_.push_back(request);
}

bool RequestTracker::completeRequest(
    const QByteArray& ct_hash, quint32 offset, quint32 size, Peer* peer) {
  return removeRequests(
      [=](const BlockRequest& req) {
        return req.ct_hash == ct_hash && req.offset == offset && req.size == size &&
               req.peer == peer;
      },
      "Block downloaded");
}

void RequestTracker::removeExpired() {
  removeRequests(
      [=](const BlockRequest& req) { return req.expires <= std::chrono::steady_clock::now(); },
      "Request timed out");
}

void RequestTracker::peerDisconnected(Peer* peer) {
  removeRequests([=](const BlockRequest& req) { return req.peer == peer; }, "Peer disconnected");
}

void RequestTracker::peerChoked(Peer* peer) {
  removeRequests([=](const BlockRequest& req) { return req.peer == peer; }, "Peer choked");
}

void RequestTracker::removeExisting(const QByteArray& ct_hash) {
  removeRequests(
      [=](const BlockRequest& req) { return req.ct_hash == ct_hash; }, "Chunk alredy downloaded");
}

QList<BlockRequest> RequestTracker::requestsForChunk(const QByteArray& ct_hash) {
  QList<BlockRequest> requests_chunk;
  for (const auto& request : requests_)
    if (request.ct_hash == ct_hash) requests_chunk << request;
  return requests_chunk;
}

bool RequestTracker::removeRequests(
    std::function<bool(const BlockRequest&)> pred, const QString& reason) {
  bool removed_ = false;
  QMutableLinkedListIterator<BlockRequest> request_it(requests_);
  while (request_it.hasNext()) {
    if (pred(request_it.next())) {
      removed_ = true;
      removeRequest(request_it, reason);
    }
  }
  return removed_;
}

void RequestTracker::removeRequest(
    QMutableLinkedListIterator<BlockRequest>& it, const QString& reason) {
  BlockRequest request = it.value();  // copy and remove
  it.remove();
  qCDebug(log_downloader) << "Removed request to:" << request.peer->endpoint()
                          << "for:" << request.ct_hash.toHex() << "off:" << request.offset
                          << "len:" << request.size << "Reason:" << reason;
}

}  // namespace librevault
