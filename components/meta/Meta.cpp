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
#include "ChunkInfo.h"
#include "EncryptedData.h"
#include "Meta_p.h"
#include <Secret.h>
#include <google/protobuf/util/message_differencer.h>
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

}  // namespace

Meta::Meta() { d = new MetaPrivate; }
Meta::Meta(const Meta& r) = default;
Meta::Meta(Meta&& r) noexcept = default;
Meta& Meta::operator=(const Meta& r) = default;
Meta& Meta::operator=(Meta&& r) noexcept = default;
Meta::~Meta() = default;

bool Meta::operator==(const Meta& r) const { return ::google::protobuf::util::MessageDifferencer::Equals(d->proto, r.d->proto); }

Meta::Meta(const QByteArray& meta_s) { parse(meta_s); }

bool Meta::isEmpty() const { return *this == Meta(); }

quint64 Meta::size() const {
  quint64 total_size = 0;
  for (auto& chunk : chunks()) {
    total_size += chunk.size();
  }
  return total_size;
}

QByteArray Meta::serialize() const { return QByteArray::fromStdString(d->proto.SerializeAsString()); }

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

quint64 Meta::revision() const { return d->proto.revision(); }
void Meta::revision(quint64 revision) { d->proto.set_revision(revision); }

EncryptedData Meta::path() const { return conv_encdata(d->proto.path()); }

Meta::Timestamp Meta::timestamp() const { return conv_timestamp(d->proto.timestamp()); }
void Meta::timestamp(Timestamp revision) { d->proto.mutable_timestamp()->CopyFrom(conv_timestamp(revision)); }

void Meta::path(const EncryptedData& path) { d->proto.mutable_path()->CopyFrom(conv_encdata(path)); }

Meta::Kind Meta::kind() const { return (Meta::Kind)d->proto.kind(); }
void Meta::kind(Kind meta_kind) { d->proto.set_kind(meta_kind); }

qint64 Meta::mtime() const { return conv_timestamp(d->proto.mtime()).time_since_epoch().count(); }
void Meta::mtime(qint64 mtime) { d->proto.mutable_mtime()->CopyFrom(conv_timestamp(Timestamp(std::chrono::seconds(mtime)))); }

quint64 Meta::mtimeGranularity() const { return d->proto.mtime_granularity(); }
void Meta::mtimeGranularity(quint64 mtime) { d->proto.set_mtime_granularity(mtime); }

quint32 Meta::windowsAttrib() const { return d->proto.windows_attrib(); }
void Meta::windowsAttrib(quint32 windows_attrib) { d->proto.set_windows_attrib(windows_attrib); }

quint32 Meta::mode() const { return d->proto.mode(); }
void Meta::mode(quint32 mode) { d->proto.set_mode(mode); }

quint32 Meta::uid() const { return d->proto.uid(); }
void Meta::uid(quint32 uid) { d->proto.set_uid(uid); }

quint32 Meta::gid() const { return d->proto.gid(); }
void Meta::gid(quint32 gid) { d->proto.set_gid(gid); }

quint32 Meta::minChunksize() const { return d->proto.min_chunksize(); }
void Meta::minChunksize(quint32 min_chunksize) { d->proto.set_min_chunksize(min_chunksize); }

quint32 Meta::maxChunksize() const { return d->proto.max_chunksize(); }
void Meta::maxChunksize(quint32 max_chunksize) { d->proto.set_max_chunksize(max_chunksize); }

quint64 Meta::rabinPolynomial() const { return d->proto.rabin_polynomial(); }
void Meta::rabinPolynomial(quint64 rabin_polynomial) { d->proto.set_rabin_polynomial(rabin_polynomial); }

quint32 Meta::rabinShift() const { return d->proto.rabin_shift(); }
void Meta::rabinShift(quint32 rabin_shift) { d->proto.set_rabin_shift(rabin_shift); }

quint64 Meta::rabinMask() const { return d->proto.rabin_mask(); }
void Meta::rabinMask(quint64 rabin_mask) { d->proto.rabin_mask(); }

QList<ChunkInfo> Meta::chunks() const {
  QList<ChunkInfo> result;
  for (const auto& chunk : d->proto.chunks()) {
    ChunkInfo info;
    info.ctHash(QByteArray::fromStdString(chunk.ct_hash()));
    info.size(chunk.size());
    info.iv(QByteArray::fromStdString(chunk.iv()));
    info.ptKeyedHash(QByteArray::fromStdString(chunk.pt_keyed_hash()));
    result.push_back(info);
  }
  return result;
}
void Meta::chunks(const QList<ChunkInfo>& chunks) {
  for (const auto& info : chunks) {
    serialization::ChunkInfo* chunk = d->proto.add_chunks();
    chunk->set_ct_hash(info.ctHash().toStdString());
    chunk->set_size(info.size());
    chunk->set_iv(info.iv().toStdString());
    chunk->set_pt_keyed_hash(info.ptKeyedHash().toStdString());
  }
}

EncryptedData Meta::symlinkTarget() const { return conv_encdata(d->proto.symlink_target()); }
void Meta::symlinkTarget(const EncryptedData& symlink_target) { d->proto.mutable_symlink_target()->CopyFrom(conv_encdata(symlink_target)); }

} /* namespace librevault */
