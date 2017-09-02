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
#include "Meta.h"
#include "AES_CBC.h"
#include "Meta_p.h"
#include "EncryptedData.h"
#include <QCryptographicHash>

namespace librevault {

namespace {

serialization::EncryptedData conv_encdata(const EncryptedData& enc) {
  serialization::EncryptedData enc_s;
  enc_s.set_ct(enc.ciphertext(), enc.ciphertext().size());
  enc_s.set_iv(enc.iv(), enc.iv().size());
  return enc_s;
}

EncryptedData conv_encdata(const serialization::EncryptedData& enc) {
  return EncryptedData::fromCiphertext(QByteArray::fromStdString(enc.ct()), QByteArray::fromStdString(enc.iv()));
}

inline ::google::protobuf::Timestamp conv_timestamp(Meta::Timestamp timestamp) {
  auto duration = timestamp.time_since_epoch();

  ::google::protobuf::Timestamp timestamp_pb;
  timestamp_pb.set_seconds(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
  timestamp_pb.set_nanos(duration.count() % 1000000000ll);
  return timestamp_pb;
};

inline Meta::Timestamp conv_timestamp(::google::protobuf::Timestamp timestamp) {
  return Meta::Timestamp(std::chrono::nanoseconds(timestamp.seconds() * 1000000000ll + timestamp.nanos()));
};
}

QByteArray Meta::Chunk::encrypt(QByteArray chunk, QByteArray key, QByteArray iv) { return encryptAesCbc(chunk, key, iv, chunk.size() % 16 != 0); }

QByteArray Meta::Chunk::decrypt(QByteArray chunk, uint32_t size, QByteArray key, QByteArray iv) {
  return decryptAesCbc(chunk, key, iv, chunk.size() % 16 != 0);
}

QByteArray Meta::Chunk::compute_strong_hash(QByteArray chunk) {
  QCryptographicHash hasher(QCryptographicHash::Sha3_256);
  hasher.addData(chunk);
  return hasher.result();
}

Meta::Meta() { d = new MetaPrivate; }
Meta::Meta(const Meta& r) { d = r.d; }
Meta::Meta(Meta&& r) noexcept { d = std::move(r.d); }

Meta& Meta::operator=(const Meta& r) {
  d = r.d;
  return *this;
}
Meta& Meta::operator=(Meta&& r) noexcept {
  d = std::move(r.d);
  return *this;
}

Meta::Meta(QByteArray meta_s) { parse(meta_s); }
Meta::~Meta() {}

uint64_t Meta::size() const {
  uint64_t total_size = 0;
  for (auto& chunk : chunks()) {
    total_size += chunk.size;
  }
  return total_size;
}

QByteArray Meta::serialize() const {
  return QByteArray::fromStdString(d->proto.SerializeAsString());
}

void Meta::parse(const QByteArray& serialized) { d->proto.ParseFromArray(serialized, serialized.size()); }

QByteArray Meta::makePathId(QByteArray path, const Secret& secret) {
  QCryptographicHash hasher(QCryptographicHash::Sha3_256);
  hasher.addData(secret.encryptionKey());
  hasher.addData(path);
  return hasher.result();
}

//
QByteArray Meta::pathKeyedHash() const { return QByteArray::fromStdString(d->proto.path_keyed_hash()); }
void Meta::pathKeyedHash(const QByteArray& path_id) { d->proto.set_path_keyed_hash(path_id.toStdString()); }

EncryptedData Meta::path() const {
  return conv_encdata(d->proto.path());
}

void Meta::path(const EncryptedData& path)  {
  d->proto.mutable_path()->CopyFrom(conv_encdata(path));
}

Meta::Kind Meta::kind() const { return (Meta::Kind)d->proto.kind(); }
void Meta::kind(Kind meta_kind) { d->proto.set_kind(meta_kind); }

Meta::Timestamp Meta::timestamp() const { return conv_timestamp(d->proto.timestamp()); }
void Meta::timestamp(Timestamp revision) { d->proto.mutable_timestamp()->CopyFrom(conv_timestamp(revision)); }

int64_t Meta::mtime() const { return conv_timestamp(d->proto.mtime()).time_since_epoch().count(); }
void Meta::set_mtime(int64_t mtime) { d->proto.mutable_mtime()->CopyFrom(conv_timestamp(Timestamp(std::chrono::seconds(mtime)))); }

uint32_t Meta::windows_attrib() const { return d->proto.windows_attrib(); }
void Meta::set_windows_attrib(uint32_t windows_attrib) { d->proto.set_windows_attrib(windows_attrib); }

uint32_t Meta::mode() const { return d->proto.mode(); }
void Meta::set_mode(uint32_t mode) { d->proto.set_mode(mode); }

uint32_t Meta::uid() const { return d->proto.uid(); }
void Meta::set_uid(uint32_t uid) { d->proto.set_uid(uid); }

uint32_t Meta::gid() const { return d->proto.gid(); }
void Meta::set_gid(uint32_t gid) { d->proto.set_gid(gid); }

uint32_t Meta::min_chunksize() const { return d->proto.min_chunksize(); }
void Meta::set_min_chunksize(uint32_t min_chunksize) { d->proto.set_min_chunksize(min_chunksize); }

uint32_t Meta::max_chunksize() const { return d->proto.max_chunksize(); }
void Meta::set_max_chunksize(uint32_t max_chunksize) { d->proto.set_max_chunksize(max_chunksize); }

quint64 Meta::rabinPolynomial() const { return d->proto.rabin_polynomial(); }
void Meta::rabinPolynomial(quint64 rabin_polynomial) { d->proto.set_rabin_polynomial(rabin_polynomial); }

quint32 Meta::rabinShift() const { return d->proto.rabin_shift(); }
void Meta::rabinShift(quint32 rabin_shift) { d->proto.set_rabin_shift(rabin_shift); }

quint64 Meta::rabinMask() const { return d->proto.rabin_mask(); }
void Meta::rabinMask(quint64 rabin_mask) { d->proto.rabin_mask(); }

QList<Meta::Chunk> Meta::chunks() const { return d->chunks_; }
void Meta::set_chunks(QList<Meta::Chunk> chunks) { d->chunks_ = chunks; }

EncryptedData Meta::symlinkTarget() const {
  return conv_encdata(d->proto.symlink_target());
}
void Meta::symlinkTarget(const EncryptedData& symlink_target) {
  d->proto.mutable_symlink_target()->CopyFrom(conv_encdata(symlink_target));
}

} /* namespace librevault */
