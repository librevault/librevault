#include "EncStorage.h"

#include "ChunkStorage.h"
#include "control/FolderParams.h"
#include "crypto/Base32.h"

namespace librevault {

EncStorage::EncStorage(const FolderParams& params, QObject* parent) : QObject(parent), params_(params) {}

QString EncStorage::make_chunk_ct_name(const QByteArray& ct_hash) const noexcept {
  return "chunk-" + QString::fromLatin1(ct_hash | crypto::Base32());
}

QString EncStorage::make_chunk_ct_path(const QByteArray& ct_hash) const noexcept {
  return params_.system_path + "/" + make_chunk_ct_name(ct_hash);
}

bool EncStorage::have_chunk(const QByteArray& ct_hash) const noexcept {
  QReadLocker lk(&storage_mtx_);
  return QFile::exists(make_chunk_ct_path(ct_hash));
}

QByteArray EncStorage::get_chunk(const QByteArray& ct_hash) const {
  QReadLocker lk(&storage_mtx_);

  QFile chunk_file(make_chunk_ct_path(ct_hash));
  if (!chunk_file.open(QIODevice::ReadOnly)) throw ChunkStorage::ChunkNotFound();

  return chunk_file.readAll();
}

void EncStorage::put_chunk(const QByteArray& ct_hash, QFile* chunk_f) {
  QWriteLocker lk(&storage_mtx_);

  chunk_f->setParent(this);
  chunk_f->rename(make_chunk_ct_path(ct_hash));
  chunk_f->deleteLater();

  LOGD("Encrypted block" << ct_hash.toHex() << "pushed into EncStorage");
}

void EncStorage::remove_chunk(const QByteArray& ct_hash) {
  QWriteLocker lk(&storage_mtx_);
  QFile::remove(make_chunk_ct_path(ct_hash));

  LOGD("Block" << ct_hash.toHex() << "removed from EncStorage");
}

}  // namespace librevault
