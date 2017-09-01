/* Copyright (C) 2015 Alexander Shishenko <alex@shishenko.com>
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

#include "Secret.h"
#include "EncryptedData.h"
#include <QList>
#include <chrono>
#include <QtCore/QSharedDataPointer>

namespace librevault {

class ChunkInfo;

class InodePrivate;
class Inode {
public:
  using Timestamp = std::chrono::system_clock::time_point;
  enum Kind : quint32 {
    FILE = 0, DIRECTORY = 1, SYMLINK = 2, DELETED = 255
  };

  QByteArray pathKeyedHash() const;
  void pathKeyedHash(const QByteArray& path_keyed_hash);

  EncryptedData path() const;
  void path(const EncryptedData& path);

  Timestamp timestamp() const;
  void timestamp(Timestamp timestamp);

  Kind kind() const;
  void kind(Kind kind);

  Timestamp mtime() const;
  void mtime(Timestamp mtime);

  std::chrono::nanoseconds mtimeGranularity() const;
  void mtimeGranularity(std::chrono::nanoseconds mtime_granularity);

  quint32 windowsAttrib() const;
  void windowsAttrib(quint32 windows_attrib);

  quint32 mode() const;
  void mode(quint32 mode);

  quint32 uid() const;
  void uid(quint32 uid);

  quint32 gid() const;
  void gid(quint32 gid);

  quint32 maxChunksize() const;
  void maxChunksize(quint32 max_chunksize);

  quint32 minChunksize() const;
  void minChunksize(quint32 min_chunksize);

  quint64 rabinPolynomial() const;
  void rabinPolynomial(quint64 rabin_polynomial);

  quint32 rabinShift() const;
  void rabinShift(quint32 rabin_shift);

  quint64 rabinMask() const;
  void rabinMask(quint64 rabin_mask);

  QList<ChunkInfo> chunks() const;
  void chunks(const QList<ChunkInfo>& chunks);

  EncryptedData symlinkTarget() const;
  void symlinkTarget(const EncryptedData& symlink_target);

private:
  QSharedDataPointer<InodePrivate> d;
};

} /* namespace librevault */
