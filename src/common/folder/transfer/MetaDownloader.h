#pragma once
#include <QObject>

#include "SignedMeta.h"
#include "util/conv_bitfield.h"
#include "util/log.h"

namespace librevault {

class RemoteFolder;
class MetaStorage;
class Downloader;

class MetaDownloader : public QObject {
  Q_OBJECT
  LOG_SCOPE("MetaDownloader");

 public:
  MetaDownloader(MetaStorage* meta_storage, Downloader* downloader, QObject* parent);

  /* Message handlers */
  void handle_have_meta(RemoteFolder* origin, const Meta::PathRevision& revision, const QBitArray& bitfield);
  void handle_meta_reply(RemoteFolder* origin, const SignedMeta& smeta, const QBitArray& bitfield);

 private:
  MetaStorage* meta_storage_;
  Downloader* downloader_;
};

}  // namespace librevault
