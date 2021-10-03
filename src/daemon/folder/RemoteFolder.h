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

#include "Meta.h"
#include "SignedMeta.h"
#include "util/blob.h"
#include "util/conv_bitfield.h"

namespace librevault {

class RemoteFolder : public QObject {
  Q_OBJECT
  friend class FolderGroup;

 signals:
  void handshakeSuccess();
  void handshakeFailed();

  /* Message signals */
  void rcvdChoke();
  void rcvdUnchoke();
  void rcvdInterested();
  void rcvdNotInterested();

  void rcvdHaveMeta(Meta::PathRevision, bitfield_type);
  void rcvdHaveChunk(blob);

  void rcvdMetaRequest(Meta::PathRevision);
  void rcvdMetaReply(SignedMeta, bitfield_type);

  void rcvdBlockRequest(blob, uint32_t, uint32_t);
  void rcvdBlockReply(blob, uint32_t, blob);

 public:
  RemoteFolder(QObject* parent);
  virtual ~RemoteFolder();

  virtual QString displayName() const = 0;
  virtual QJsonObject collect_state() = 0;
  QString log_tag() const;

  /* Message senders */
  virtual void choke() = 0;
  virtual void unchoke() = 0;
  virtual void interest() = 0;
  virtual void uninterest() = 0;

  virtual void post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield) = 0;
  virtual void post_have_chunk(const blob& ct_hash) = 0;

  virtual void request_meta(const Meta::PathRevision& revision) = 0;
  virtual void post_meta(const SignedMeta& smeta, const bitfield_type& bitfield) = 0;

  virtual void request_block(const blob& ct_hash, uint32_t offset, uint32_t size) = 0;
  virtual void post_block(const blob& ct_hash, uint32_t offset, const blob& chunk) = 0;

  /* High-level RAII wrappers */
  struct InterestGuard {
    InterestGuard(RemoteFolder* remote);
    ~InterestGuard();

   private:
    RemoteFolder* remote_;
  };
  std::shared_ptr<InterestGuard> get_interest_guard();

  /* Getters */
  bool am_choking() const { return am_choking_; }
  bool am_interested() const { return am_interested_; }
  bool peer_choking() const { return peer_choking_; }
  bool peer_interested() const { return peer_interested_; }

  virtual bool ready() const = 0;

 protected:
  bool am_choking_ = true;
  bool am_interested_ = false;
  bool peer_choking_ = true;
  bool peer_interested_ = false;

  std::weak_ptr<InterestGuard> interest_guard_;
};

}  // namespace librevault
