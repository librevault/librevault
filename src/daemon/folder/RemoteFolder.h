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

  void rcvdHaveMeta(Meta::PathRevision, QBitArray);
  void rcvdHaveChunk(QByteArray);

  void rcvdMetaRequest(Meta::PathRevision);
  void rcvdMetaReply(SignedMeta, QBitArray);

  void rcvdBlockRequest(QByteArray, uint32_t, uint32_t);
  void rcvdBlockReply(QByteArray, uint32_t, QByteArray);

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

  virtual void post_have_meta(const Meta::PathRevision& revision, const QBitArray& bitfield) = 0;
  virtual void post_have_chunk(const QByteArray& ct_hash) = 0;

  virtual void request_meta(const Meta::PathRevision& revision) = 0;
  virtual void post_meta(const SignedMeta& smeta, const QBitArray& bitfield) = 0;

  virtual void request_block(const QByteArray& ct_hash, uint32_t offset, uint32_t size) = 0;
  virtual void post_block(const QByteArray& ct_hash, uint32_t offset, const QByteArray& chunk) = 0;

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
