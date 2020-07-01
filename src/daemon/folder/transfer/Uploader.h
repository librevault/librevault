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
  Uploader(ChunkStorage* chunk_storage, QObject* parent);

  void broadcast_chunk(QList<RemoteFolder*> remotes, const QByteArray& ct_hash);

  /* Message handlers */
  void handle_interested(RemoteFolder* remote);
  void handle_not_interested(RemoteFolder* remote);

  void handle_block_request(RemoteFolder* remote, const QByteArray& ct_hash, uint32_t offset, uint32_t size) noexcept;

 private:
  ChunkStorage* chunk_storage_;

  QByteArray get_block(const QByteArray& ct_hash, uint32_t offset, uint32_t size);
};

}  // namespace librevault
