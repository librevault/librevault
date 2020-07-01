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

  void broadcast_meta(QList<RemoteFolder*> remotes, const Meta::PathRevision& revision, const QBitArray& bitfield);

  /* Message handlers */
  void handle_handshake(RemoteFolder* remote);
  void handle_meta_request(RemoteFolder* remote, const Meta::PathRevision& revision);

 private:
  MetaStorage* meta_storage_;
  ChunkStorage* chunk_storage_;
};

}  // namespace librevault
