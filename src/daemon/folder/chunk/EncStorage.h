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
 */
#pragma once
#include <QFile>
#include <QReadWriteLock>
#include <memory>

#include "util/blob.h"
#include "util/log.h"

namespace librevault {

class FolderParams;
class EncStorage : public QObject {
  Q_OBJECT
  LOG_SCOPE("EncStorage");

 public:
  EncStorage(const FolderParams& params, QObject* parent);

  bool have_chunk(const QByteArray& ct_hash) const noexcept;
  QByteArray get_chunk(const QByteArray& ct_hash) const;
  void put_chunk(const QByteArray& ct_hash, QFile* chunk_f);
  void remove_chunk(const QByteArray& ct_hash);

 private:
  const FolderParams& params_;
  mutable QReadWriteLock storage_mtx_;

  QString make_chunk_ct_name(const QByteArray& ct_hash) const noexcept;
  QString make_chunk_ct_path(const QByteArray& ct_hash) const noexcept;
};

}  // namespace librevault
