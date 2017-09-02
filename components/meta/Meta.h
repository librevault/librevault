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

#include "util/AES_CBC_DATA.h"
#include "EncryptedData.h"
#include <QList>
#include <chrono>

namespace librevault {

class MetaPrivate;
class Meta {
 public:
  using Timestamp = std::chrono::system_clock::time_point;
  enum Kind : uint32_t { FILE = 0, DIRECTORY = 1, SYMLINK = 2, /*STREAM = 3,*/ DELETED = 255 };
  enum StrongHashType : uint8_t { SHA3_256 = 0 };
  struct Chunk {
    QByteArray ct_hash;
    uint32_t size;
    QByteArray iv;

    QByteArray pt_hmac;

    static QByteArray encrypt(QByteArray chunk, QByteArray key, QByteArray iv);
    static QByteArray decrypt(QByteArray chunk, uint32_t size, QByteArray key, QByteArray iv);

    static QByteArray compute_strong_hash(QByteArray chunk);
  };

 private:
  QSharedDataPointer<MetaPrivate> d;

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
    int64_t revision_;
  };

  /* Class methods */
  Meta();
  Meta(const Meta& r);
  Meta(Meta&& r) noexcept;
  explicit Meta(QByteArray meta_s);
  virtual ~Meta();

  Meta& operator=(const Meta& r);
  Meta& operator=(Meta&& r) noexcept;

  /* Serialization */
  QByteArray serialize() const;
  void parse(const QByteArray& serialized);

  /* Generators */
  static QByteArray makePathId(QByteArray path, const Secret& secret);

  /* Smart getters+setters */
  PathRevision path_revision() const { return PathRevision{pathKeyedHash(), timestamp().time_since_epoch().count()}; }
  uint64_t size() const;

  // Dumb getters & setters
  QByteArray pathKeyedHash() const;
  void pathKeyedHash(const QByteArray& path_id);

  EncryptedData path() const;
  void path(const EncryptedData& path) ;

  Kind kind() const;
  void kind(Kind meta_kind);

  Timestamp timestamp() const;
  void timestamp(Timestamp revision);

  int64_t mtime() const;
  void set_mtime(int64_t mtime);

  uint32_t windows_attrib() const;
  void set_windows_attrib(uint32_t windows_attrib);

  uint32_t mode() const;
  void set_mode(uint32_t mode);

  uint32_t uid() const;
  void set_uid(uint32_t uid);

  uint32_t gid() const;
  void set_gid(uint32_t gid);

  uint32_t min_chunksize() const;
  void set_min_chunksize(uint32_t min_chunksize);

  uint32_t max_chunksize() const;
  void set_max_chunksize(uint32_t max_chunksize);

  quint64 rabinPolynomial() const;
  void rabinPolynomial(quint64 rabin_polynomial);

  quint32 rabinShift() const;
  void rabinShift(quint32 rabin_shift);

  quint64 rabinMask() const;
  void rabinMask(quint64 rabin_mask);

  QList<Chunk> chunks() const;
  void set_chunks(QList<Chunk> chunks);

  EncryptedData symlinkTarget() const;
  void symlinkTarget(const EncryptedData& symlink_target);
};

} /* namespace librevault */
