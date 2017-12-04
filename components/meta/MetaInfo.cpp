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
#include "MetaInfo.h"
#include "crypto/AES_CBC.h"
#include "ChunkInfo.h"
#include "EncryptedData.h"
#include "MetaInfo_p.h"
#include "secret/Secret.h"
#include <google/protobuf/util/json_util.h>
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

inline ::google::protobuf::Timestamp conv_timestamp(MetaInfo::Timestamp timestamp) {
  auto duration = timestamp.time_since_epoch();

  ::google::protobuf::Timestamp timestamp_pb;
  timestamp_pb.set_seconds(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
  timestamp_pb.set_nanos(duration.count() % 1000000000ll);
  return timestamp_pb;
};

inline MetaInfo::Timestamp conv_timestamp(::google::protobuf::Timestamp timestamp) {
  return MetaInfo::Timestamp(std::chrono::nanoseconds(timestamp.seconds() * 1000000000ll + timestamp.nanos()));
};

}  // namespace

MetaInfo::MetaInfo() { d = new MetaPrivate; }
MetaInfo::MetaInfo(const MetaInfo& r) = default;
MetaInfo::MetaInfo(MetaInfo&& r) noexcept = default;
MetaInfo& MetaInfo::operator=(const MetaInfo& r) = default;
MetaInfo& MetaInfo::operator=(MetaInfo&& r) noexcept = default;
MetaInfo::~MetaInfo() = default;

bool MetaInfo::operator==(const MetaInfo& r) const { return ::google::protobuf::util::MessageDifferencer::Equals(d->proto, r.d->proto); }

MetaInfo::MetaInfo(const QByteArray& meta_s) : MetaInfo() { parseFromBinary(meta_s); }

bool MetaInfo::isEmpty() const { return *this == MetaInfo(); }

quint64 MetaInfo::size() const {
  quint64 total_size = 0;
  for (auto& chunk : chunks())
    total_size += chunk.size();
  return total_size;
}

QByteArray MetaInfo::serializeToBinary() const { return QByteArray::fromStdString(d->proto.SerializeAsString()); }

void MetaInfo::parseFromBinary(const QByteArray &serialized) {
  if(! d->proto.ParseFromArray(serialized, serialized.size()))
    throw parse_error();
}

QJsonDocument MetaInfo::serializeToJson() const {
  std::string result;

  ::google::protobuf::util::JsonPrintOptions options;
  options.add_whitespace = false;
  options.always_print_primitive_fields = true;
  ::google::protobuf::util::MessageToJsonString(d->proto, &result, options);

  return QJsonDocument::fromJson(QByteArray::fromStdString(result));
}

void MetaInfo::parseFromJson(const QJsonDocument& json) {
  ::google::protobuf::util::JsonParseOptions options;
  options.ignore_unknown_fields = true;
  ::google::protobuf::util::JsonStringToMessage(json.toJson(QJsonDocument::Compact).toStdString(), &(d->proto), options);
}

//
QByteArray MetaInfo::pathKeyedHash() const { return QByteArray::fromStdString(d->proto.path_keyed_hash()); }
void MetaInfo::pathKeyedHash(const QByteArray& path_id) { d->proto.set_path_keyed_hash(path_id.toStdString()); }

quint64 MetaInfo::revision() const { return d->proto.revision(); }
void MetaInfo::revision(quint64 revision) { d->proto.set_revision(revision); }

MetaInfo::Timestamp MetaInfo::timestamp() const { return conv_timestamp(d->proto.timestamp()); }
void MetaInfo::timestamp(Timestamp revision) { d->proto.mutable_timestamp()->CopyFrom(conv_timestamp(revision)); }

EncryptedData MetaInfo::path() const { return conv_encdata(d->proto.path()); }
void MetaInfo::path(const EncryptedData& path) { d->proto.mutable_path()->CopyFrom(conv_encdata(path)); }

MetaInfo::Kind MetaInfo::kind() const { return (MetaInfo::Kind)d->proto.kind(); }
void MetaInfo::kind(Kind meta_kind) { d->proto.set_kind(meta_kind); }

qint64 MetaInfo::mtime() const { return conv_timestamp(d->proto.mtime()).time_since_epoch().count(); }
void MetaInfo::mtime(qint64 mtime) { d->proto.mutable_mtime()->CopyFrom(conv_timestamp(Timestamp(std::chrono::nanoseconds(mtime)))); }

quint64 MetaInfo::mtimeGranularity() const { return d->proto.mtime_granularity(); }
void MetaInfo::mtimeGranularity(quint64 mtime) { d->proto.set_mtime_granularity(mtime); }

quint32 MetaInfo::windowsAttrib() const { return d->proto.windows_attrib(); }
void MetaInfo::windowsAttrib(quint32 windows_attrib) { d->proto.set_windows_attrib(windows_attrib); }

quint32 MetaInfo::mode() const { return d->proto.mode(); }
void MetaInfo::mode(quint32 mode) { d->proto.set_mode(mode); }

quint32 MetaInfo::uid() const { return d->proto.uid(); }
void MetaInfo::uid(quint32 uid) { d->proto.set_uid(uid); }

quint32 MetaInfo::gid() const { return d->proto.gid(); }
void MetaInfo::gid(quint32 gid) { d->proto.set_gid(gid); }

quint32 MetaInfo::minChunksize() const { return d->proto.min_chunksize(); }
void MetaInfo::minChunksize(quint32 min_chunksize) { d->proto.set_min_chunksize(min_chunksize); }

quint32 MetaInfo::maxChunksize() const { return d->proto.max_chunksize(); }
void MetaInfo::maxChunksize(quint32 max_chunksize) { d->proto.set_max_chunksize(max_chunksize); }

quint64 MetaInfo::rabinPolynomial() const { return d->proto.rabin_polynomial(); }
void MetaInfo::rabinPolynomial(quint64 rabin_polynomial) { d->proto.set_rabin_polynomial(rabin_polynomial); }

quint32 MetaInfo::rabinShift() const { return d->proto.rabin_shift(); }
void MetaInfo::rabinShift(quint32 rabin_shift) { d->proto.set_rabin_shift(rabin_shift); }

quint64 MetaInfo::rabinMask() const { return d->proto.rabin_mask(); }
void MetaInfo::rabinMask(quint64 rabin_mask) { d->proto.rabin_mask(); }

QList<ChunkInfo> MetaInfo::chunks() const {
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
void MetaInfo::chunks(const QList<ChunkInfo>& chunks) {
  d->proto.clear_chunks();
  for (const auto& info : chunks) {
    serialization::ChunkInfo* chunk = d->proto.add_chunks();
    chunk->set_ct_hash(info.ctHash().toStdString());
    chunk->set_size(info.size());
    chunk->set_iv(info.iv().toStdString());
    chunk->set_pt_keyed_hash(info.ptKeyedHash().toStdString());
  }
}

EncryptedData MetaInfo::symlinkTarget() const { return conv_encdata(d->proto.symlink_target()); }
void MetaInfo::symlinkTarget(const EncryptedData& symlink_target) { d->proto.mutable_symlink_target()->CopyFrom(conv_encdata(symlink_target)); }

QDebug operator<<(QDebug debug, const MetaInfo::PathRevision& id) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "PathRevision(path_keyed_hash: " << id.path_keyed_hash_.toHex()
                  << ", revision:" << id.revision_ << ")";
  return debug;
}

}  // namespace librevault
