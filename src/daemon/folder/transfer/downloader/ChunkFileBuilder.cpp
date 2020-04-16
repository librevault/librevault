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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
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
