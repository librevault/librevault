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
#include <Meta.h>
#include <Meta_s.pb.h>

#include "crypto/AES_CBC.h"
#include "crypto/KMAC-SHA3.h"
#include "crypto/SHA.h"

namespace librevault {

QByteArray Meta::Chunk::encrypt(const QByteArray& chunk, const QByteArray& key, const QByteArray& iv) {
  return chunk | crypto::AES_CBC(key, iv);
}

QByteArray Meta::Chunk::decrypt(const QByteArray& chunk, uint32_t size, const QByteArray& key, const QByteArray& iv) {
  return chunk | crypto::De<crypto::AES_CBC>(key, iv);
}

QByteArray Meta::Chunk::computeStrongHash(const QByteArray& chunk, StrongHashType type) {
  switch (type) {
    case SHA3_224:
      return chunk | crypto::SHA3(224);
    case SHA2_224:
      return chunk | crypto::SHA2(224);
    default:
      return QByteArray();  // TODO: throw some exception.
  }
}

Meta::Meta() = default;
Meta::Meta(const QByteArray& meta_s) { parse(meta_s); }
Meta::~Meta() = default;

bool Meta::validate() const {
  if (path_id_.size() != 28) return false;  // Hash size mismatch
  if (!path_.check()) return false;         // Data/IV size mismatch

  if (meta_type_ == Type::SYMLINK && !symlink_path_.check()) return false;

  if (meta_type_ == Type::FILE) {
    if (algorithm_type_ != RABIN || strong_hash_type_ > SHA2_224  // Unknown cryptographic hashing algorithm
        || max_chunksize_ == 0 || min_chunksize_ == 0 || max_chunksize_ > min_chunksize_)
      return false;  // Invalid chunk constraints

    for (auto& chunk : chunks()) {
      if (chunk.size > max_chunksize_ || chunk.size == 0)  // Broken chunk constraint
        if (chunk.ct_hash.size() != 28) return false;
      if (chunk.iv.size() != 16) return false;

      if (chunk.pt_hmac.size() != 28) return false;
    }
  }

  return true;
}

/*bool Meta::validate(const Secret& secret) const {
        if(! validate()) return false;  // "Easy" validation failed

        if(make_path_id(path(secret), secret) != path_id_) return false;    // path_id is inconsistent with
encrypted_path and its iv

        try {
                auto params = rabin_global_params(secret);
                // TODO: Check rabin parameters somehow
        }catch(parse_error& e){
                return false;
        }

        return true;
}*/

uint64_t Meta::size() const {
  uint64_t total_size = 0;
  for (auto& chunk : chunks()) total_size += chunk.size;
  return total_size;
}

QByteArray Meta::path(const Secret& secret) const { return path_.get_plain(secret); }
void Meta::set_path(const QByteArray& path, const Secret& secret) {
  set_path_id(make_path_id(path, secret));
  path_.set_plain(path, secret);
}

QByteArray Meta::symlink_path(const Secret& secret) const { return symlink_path_.get_plain(secret); }
void Meta::set_symlink_path(std::string path, const Secret& secret) {
  symlink_path_.set_plain(QByteArray::fromStdString(path), secret);
}

QByteArray Meta::serialize() const {
  serialization::Meta meta_s;

  meta_s.set_path_id(path_id_, path_id_.size());
  meta_s.mutable_path()->set_ct(path_.ct().toStdString());
  meta_s.mutable_path()->set_iv(path_.iv().toStdString());
  meta_s.set_meta_type((uint32_t)meta_type_);
  meta_s.set_revision(revision_);

  if (meta_type_ != DELETED) {
    meta_s.mutable_generic_metadata()->set_mtime(mtime_);
    meta_s.mutable_generic_metadata()->set_windows_attrib(windows_attrib_);
    meta_s.mutable_generic_metadata()->set_mode(mode_);
    meta_s.mutable_generic_metadata()->set_uid(uid_);
    meta_s.mutable_generic_metadata()->set_gid(gid_);
  }

  if (meta_type_ == SYMLINK) {
    meta_s.mutable_symlink_metadata()->mutable_symlink_path()->set_ct(symlink_path_.ct().toStdString());
    meta_s.mutable_symlink_metadata()->mutable_symlink_path()->set_iv(symlink_path_.iv().toStdString());
  }

  if (meta_type_ == FILE) {
    auto file_metadata = meta_s.mutable_file_metadata();
    file_metadata->set_algorithm_type((uint32_t)algorithm_type_);
    file_metadata->set_strong_hash_type((uint32_t)strong_hash_type_);

    file_metadata->set_max_chunksize(max_chunksize_);
    file_metadata->set_min_chunksize(min_chunksize_);

    for (auto& chunk : chunks()) {
      auto chunk_s = file_metadata->add_chunks();
      chunk_s->set_ct_hash(chunk.ct_hash, chunk.ct_hash.size());
      chunk_s->set_size(chunk.size);
      chunk_s->set_iv(chunk.iv, chunk.iv.size());

      chunk_s->set_pt_hmac(chunk.pt_hmac, chunk.pt_hmac.size());
    }
  }

  QByteArray serialized_data(meta_s.ByteSizeLong(), 0);
  meta_s.SerializeToArray(serialized_data.data(), serialized_data.size());
  return serialized_data;
}

void Meta::parse(const QByteArray& serialized_data) {
  serialization::Meta meta_s;

  bool parsed_well = meta_s.ParseFromArray(serialized_data, serialized_data.size());
  if (!parsed_well) throw parse_error("Parse error: Protobuf parsing failed");

  path_id_ = QByteArray::fromStdString(meta_s.path_id());
  path_.setEncrypted(meta_s.path().ct(), meta_s.path().iv());
  meta_type_ = (Type)meta_s.meta_type();
  revision_ = (int64_t)meta_s.revision();

  if (meta_type_ != DELETED) {
    mtime_ = meta_s.generic_metadata().mtime();
    windows_attrib_ = meta_s.generic_metadata().windows_attrib();
    mode_ = meta_s.generic_metadata().mode();
    uid_ = meta_s.generic_metadata().uid();
    gid_ = meta_s.generic_metadata().gid();
  } else if (meta_s.type_specific_metadata_case() != 0)
    throw parse_error("Parse error: Redundant type specific metadata found on DELETED");

  if (meta_type_ == SYMLINK) {
    if (meta_s.type_specific_metadata_case() != meta_s.kSymlinkMetadata)
      throw parse_error("Parse error: Symlink metadata needed");
    symlink_path_.setEncrypted(meta_s.symlink_metadata().symlink_path().ct(),
                               meta_s.symlink_metadata().symlink_path().iv());
  }

  if (meta_type_ == FILE) {
    if (meta_s.type_specific_metadata_case() != meta_s.kFileMetadata)
      throw parse_error("Parse error: File metadata needed");
    algorithm_type_ = (AlgorithmType)meta_s.file_metadata().algorithm_type();
    strong_hash_type_ = (StrongHashType)meta_s.file_metadata().strong_hash_type();
    max_chunksize_ = meta_s.file_metadata().max_chunksize();
    min_chunksize_ = meta_s.file_metadata().min_chunksize();

    for (auto& chunk_s : meta_s.file_metadata().chunks()) {
      Chunk chunk;
      chunk.ct_hash = QByteArray::fromStdString(chunk_s.ct_hash());
      chunk.size = chunk_s.size();
      chunk.iv = QByteArray::fromStdString(chunk_s.iv());
      chunk.pt_hmac = QByteArray::fromStdString(chunk_s.pt_hmac());

      chunks_.push_back(chunk);
    }
  }
}

QByteArray Meta::make_path_id(const QByteArray& path, const Secret& secret) {
  return path | crypto::KMAC_SHA3_224(secret.get_Encryption_Key());
}

}  // namespace librevault
