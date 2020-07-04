#include "Uploader.h"

#include "folder/RemoteFolder.h"
#include "folder/chunk/ChunkStorage.h"

namespace librevault {

Uploader::Uploader(ChunkStorage* chunk_storage, QObject* parent) : QObject(parent), chunk_storage_(chunk_storage) {
  LOGFUNC();
}

void Uploader::broadcast_chunk(QList<RemoteFolder*> remotes, const QByteArray& ct_hash) {
  for (auto& remote : remotes) remote->postHaveChunk(ct_hash);
}

void Uploader::handle_interested(RemoteFolder* remote) {
  LOGFUNC();

  // TODO: write good choking algorithm.
  remote->unchoke();
}
void Uploader::handle_not_interested(RemoteFolder* remote) {
  LOGFUNC();

  // TODO: write good choking algorithm.
  remote->choke();
}

void Uploader::handle_block_request(RemoteFolder* remote, const QByteArray& ct_hash, uint32_t offset,
                                    uint32_t size) noexcept {
  try {
    if (!remote->am_choking() && remote->peer_interested())
      remote->postBlock(ct_hash, offset, get_block(ct_hash, offset, size));
  } catch (ChunkStorage::ChunkNotFound& e) {
    LOGW("Requested nonexistent block");
  }
}

QByteArray Uploader::get_block(const QByteArray& ct_hash, uint32_t offset, uint32_t size) {
  auto chunk = chunk_storage_->get_chunk(ct_hash);
  if ((int)offset < chunk.size() && (int)size <= chunk.size() - (int)offset)
    return chunk.mid(offset, size);
  else
    throw ChunkStorage::ChunkNotFound();
}

}  // namespace librevault
