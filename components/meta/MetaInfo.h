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

#include "EncryptedData.h"
#include <QList>
#include <QSharedDataPointer>
#include <chrono>

namespace librevault {

class ChunkInfo;
class Secret;

class MetaPrivate;
class MetaInfo {
 public:
  using Timestamp = std::chrono::system_clock::time_point;
  enum Kind : quint32 { FILE = 0, DIRECTORY = 1, SYMLINK = 2, /*STREAM = 3,*/ DELETED = 255 };

 public:
  /* Nested structs & classes */
  struct error : std::runtime_error {
    error(const char* what) : std::runtime_error(what) {}
    error() : error("Meta error") {}
  };

  struct parse_error : error {
    parse_error(const char* what) : error(what) {}
    parse_error() : error("Parse error") {}
  };

  /// Used for querying specific version of Meta
  struct PathRevision {
    QByteArray path_id_;
    quint64 revision_;
  };

  /* Class methods */
  MetaInfo();
  MetaInfo(const MetaInfo& r);
  MetaInfo(MetaInfo&& r) noexcept;
  explicit MetaInfo(const QByteArray& meta_s);
  virtual ~MetaInfo();

  MetaInfo& operator=(const MetaInfo& r);
  MetaInfo& operator=(MetaInfo&& r) noexcept;

  bool operator==(const MetaInfo& r) const;

  /* Serialization */
  QByteArray serialize() const;
  void parse(const QByteArray& serialized);

  /* Generators */
  static QByteArray makePathId(QByteArray path, const Secret& secret);

  /* Smart getters+setters */
  bool isEmpty() const;
  PathRevision path_revision() const { return PathRevision{pathKeyedHash(), revision()}; }
  quint64 size() const;

  // Dumb getters & setters
  QByteArray pathKeyedHash() const;
  void pathKeyedHash(const QByteArray& path_id);

  quint64 revision() const;
  void revision(quint64 revision);

  Timestamp timestamp() const;
  void timestamp(Timestamp revision);

  EncryptedData path() const;
  void path(const EncryptedData& path);

  Kind kind() const;
  void kind(Kind meta_kind);

  qint64 mtime() const;
  void mtime(qint64 mtime);

  quint64 mtimeGranularity() const;
  void mtimeGranularity(quint64 mtime);

  quint32 windowsAttrib() const;
  void windowsAttrib(quint32 windows_attrib);

  quint32 mode() const;
  void mode(quint32 mode);

  quint32 uid() const;
  void uid(quint32 uid);

  quint32 gid() const;
  void gid(quint32 gid);

  quint32 minChunksize() const;
  void minChunksize(quint32 min_chunksize);

  quint32 maxChunksize() const;
  void maxChunksize(quint32 max_chunksize);

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
  QSharedDataPointer<MetaPrivate> d;
};

} /* namespace librevault */
