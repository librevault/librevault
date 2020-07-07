#include "ChunkFileBuilder.h"

#include <QLoggingCategory>

#include "crypto/Base32.h"

namespace librevault {

Q_DECLARE_LOGGING_CATEGORY(log_downloader)

/* ChunkFileBuilderFdPool */
QFile* ChunkFileBuilderFdPool::getFile(QString path, bool release) {
  QFile* f = !release ? opened_files_[path] : opened_files_.take(path);
  if (f) return f;

  f = new QFile(path);
  if (!f->open(QIODevice::ReadWrite))
    qCWarning(log_downloader) << "Could not open" << path << "Error:" << f->errorString();
  opened_files_.insert(path, f);

  return f;
}

/* ChunkFileBuilder */
ChunkFileBuilder::ChunkFileBuilder(QString system_path, QByteArray ct_hash, quint32 size) : file_map_(size) {
  chunk_location_ = system_path + "/incomplete-" + (ct_hash | crypto::Base32());

  QFile f(chunk_location_);
  f.open(QIODevice::WriteOnly | QIODevice::Truncate);
  f.resize(size);
}

ChunkFileBuilder::~ChunkFileBuilder() {
  if (!chunk_location_.isEmpty()) QFile::remove(chunk_location_);
}

QFile* ChunkFileBuilder::release_chunk() {
  QFile* f = ChunkFileBuilderFdPool::get_instance()->getFile(chunk_location_, true);
  chunk_location_.clear();
  return f;
}

void ChunkFileBuilder::put_block(quint32 offset, const QByteArray& content) {
  auto inserted = file_map_.insert({offset, content.size()}).second;
  if (inserted) {
    QFile* f = ChunkFileBuilderFdPool::get_instance()->getFile(chunk_location_);
    if (f->pos() != offset) f->seek(offset);
    f->write(content);
  }
}

}  // namespace librevault
