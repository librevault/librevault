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
#include "Inode.h"
#include "Inode_p.h"
#include "ChunkInfo.h"
#include <QList>

namespace librevault {

namespace {

serialization::EncryptedData serialize(const EncryptedData& enc) {
  serialization::EncryptedData enc_s;
  enc_s.set_ct(enc.ciphertext(), enc.ciphertext().size());
  enc_s.set_iv(enc.iv(), enc.iv().size());
  return enc_s;
}

EncryptedData parse(const serialization::EncryptedData& enc) {
  return EncryptedData::fromCiphertext(QByteArray::fromStdString(enc.ct()), QByteArray::fromStdString(enc.iv()));
}

inline ::google::protobuf::Timestamp conv_timestamp(Inode::Timestamp timestamp) {
  auto duration = timestamp.time_since_epoch();

  ::google::protobuf::Timestamp timestamp_pb;
  timestamp_pb.set_seconds(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
  timestamp_pb.set_nanos(duration.count() % 1000000000ll);
  return timestamp_pb;
};

inline Inode::Timestamp conv_timestamp(::google::protobuf::Timestamp timestamp) {
  return Inode::Timestamp(std::chrono::nanoseconds(timestamp.seconds()*1000000000ll + timestamp.nanos()));
};

}

QByteArray Inode::pathKeyedHash() const {
  return QByteArray::fromStdString(d->proto.path_keyed_hash());
}
void Inode::pathKeyedHash(const QByteArray& path_keyed_hash) {
  d->proto.set_path_keyed_hash(path_keyed_hash.toStdString());
}

EncryptedData Inode::path() const {
  return parse(d->proto.path());
}
void Inode::path(const EncryptedData& path) {
  d->proto.mutable_path()->CopyFrom(serialize(path));
}

Inode::Timestamp Inode::timestamp() const {
  return conv_timestamp(d->proto.timestamp());
}
void Inode::timestamp(Timestamp timestamp){
  d->proto.mutable_timestamp()->CopyFrom(conv_timestamp(timestamp));
}

Inode::Kind Inode::kind() const{
  return Inode::Kind(d->proto.kind());
}
void Inode::kind(Inode::Kind kind){
  d->proto.set_kind(kind);
}

Inode::Timestamp Inode::mtime() const{
  return conv_timestamp(d->proto.mtime());
}
void Inode::mtime(Timestamp mtime){
  d->proto.mutable_mtime()->CopyFrom(conv_timestamp(mtime));

}

std::chrono::nanoseconds Inode::mtimeGranularity() const{
  return std::chrono::nanoseconds(d->proto.mtime_granularity());
}
void Inode::mtimeGranularity(std::chrono::nanoseconds mtime_granularity){
  d->proto.set_mtime_granularity(mtime_granularity.count());
}

quint32 Inode::windowsAttrib() const{
  return d->proto.windows_attrib();
}
void Inode::windowsAttrib(quint32 windows_attrib){
  d->proto.set_windows_attrib(windows_attrib);
}

quint32 Inode::mode() const{
  return d->proto.mode();
}
void Inode::mode(quint32 mode){
  d->proto.set_mode(mode);
}

quint32 Inode::uid() const{
  return d->proto.uid();
}
void Inode::uid(quint32 uid){
  d->proto.set_uid(uid);
}

quint32 Inode::gid() const{
  return d->proto.gid();
}
void Inode::gid(quint32 gid){
  d->proto.set_gid(gid);
}

quint32 Inode::maxChunksize() const{
  return d->proto.max_chunksize();
}
void Inode::maxChunksize(quint32 max_chunksize){
  d->proto.set_max_chunksize(max_chunksize);
}

quint32 Inode::minChunksize() const{
  return d->proto.min_chunksize();
}
void Inode::minChunksize(quint32 min_chunksize){
  d->proto.set_min_chunksize(min_chunksize);
}

quint64 Inode::rabinPolynomial() const{
  return d->proto.rabin_polynomial();
}
void Inode::rabinPolynomial(quint64 rabin_polynomial){
  d->proto.set_rabin_polynomial(rabin_polynomial);
}

quint32 Inode::rabinShift() const{
  return d->proto.rabin_shift();
}
void Inode::rabinShift(quint32 rabin_shift){
  d->proto.set_rabin_shift(rabin_shift);
}

quint64 Inode::rabinMask() const{
  return d->proto.rabin_mask();
}
void Inode::rabinMask(quint64 rabin_mask){
  d->proto.rabin_mask();
}

QList<ChunkInfo> Inode::chunks() const{
  QList<ChunkInfo> ret;
  ret.reserve(d->proto.chunks().size());
  for(const auto& info_s : d->proto.chunks()) {
    ChunkInfo info;
    info.size(info_s.size());
    info.iv(QByteArray::fromStdString(info_s.iv()));
    info.ptKeyedHash(QByteArray::fromStdString(info_s.pt_keyed_hash()));
    info.ctHash(QByteArray::fromStdString(info_s.ct_hash()));
    ret << info;
  }
  return ret;
}

void Inode::chunks(const QList<ChunkInfo>& chunks){
  d->proto.clear_chunks();
  for(const auto& chunk : chunks) {
    auto chunk_s = d->proto.add_chunks();
    chunk_s->set_size(chunk.size());
    chunk_s->set_iv(chunk.iv().toStdString());
    chunk_s->set_pt_keyed_hash(chunk.ptKeyedHash().toStdString());
    chunk_s->set_ct_hash(chunk.ctHash().toStdString());
  }
}

EncryptedData Inode::symlinkTarget() const{
  return parse(d->proto.symlink_target());
}
void Inode::symlinkTarget(const EncryptedData& symlink_target){
  d->proto.mutable_symlink_target()->CopyFrom(serialize(symlink_target));
}

} /* namespace librevault */
