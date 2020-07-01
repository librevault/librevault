#include "MetaUploader.h"

#include "folder/RemoteFolder.h"
#include "folder/chunk/ChunkStorage.h"
#include "folder/meta/MetaStorage.h"

namespace librevault {

MetaUploader::MetaUploader(MetaStorage* meta_storage, ChunkStorage* chunk_storage, QObject* parent)
    : QObject(parent), meta_storage_(meta_storage), chunk_storage_(chunk_storage) {
  LOGFUNC();
}

void MetaUploader::broadcast_meta(QList<RemoteFolder*> remotes, const Meta::PathRevision& revision,
                                  const QBitArray& bitfield) {
  for (auto remote : remotes) remote->post_have_meta(revision, bitfield);
}

void MetaUploader::handle_handshake(RemoteFolder* remote) {
  for (auto& meta : meta_storage_->getMeta())
    remote->post_have_meta(meta.meta().path_revision(), chunk_storage_->make_bitfield(meta.meta()));
}

void MetaUploader::handle_meta_request(RemoteFolder* remote, const Meta::PathRevision& revision) {
  try {
    remote->post_meta(meta_storage_->getMeta(revision),
                      chunk_storage_->make_bitfield(meta_storage_->getMeta(revision).meta()));
  } catch (MetaStorage::MetaNotFound& e) {
    LOGW("Requested nonexistent Meta");
  }
}

}  // namespace librevault
