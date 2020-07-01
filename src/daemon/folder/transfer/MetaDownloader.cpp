#include "MetaDownloader.h"

#include "Downloader.h"
#include "folder/FolderGroup.h"
#include "folder/RemoteFolder.h"
#include "folder/meta/MetaStorage.h"

namespace librevault {

MetaDownloader::MetaDownloader(MetaStorage* meta_storage, Downloader* downloader, QObject* parent)
    : QObject(parent), meta_storage_(meta_storage), downloader_(downloader) {
  LOGFUNC();
}

void MetaDownloader::handle_have_meta(RemoteFolder* origin, const Meta::PathRevision& revision,
                                      const QBitArray& bitfield) {
  if (meta_storage_->haveMeta(revision))
    downloader_->notifyRemoteMeta(origin, revision, bitfield);
  else if (meta_storage_->putAllowed(revision))
    origin->request_meta(revision);
  else
    LOGD("Remote node notified us about an expired Meta");
}

void MetaDownloader::handle_meta_reply(RemoteFolder* origin, const SignedMeta& smeta, const QBitArray& bitfield) {
  if (meta_storage_->putAllowed(smeta.meta().path_revision())) {
    meta_storage_->putMeta(smeta);
    downloader_->notifyRemoteMeta(origin, smeta.meta().path_revision(), bitfield);
  } else
    LOGD("Remote node posted to us about an expired Meta");
}

}  // namespace librevault
