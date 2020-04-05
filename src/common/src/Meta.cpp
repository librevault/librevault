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
#include "crypto/SHA3.h"
#include "crypto/SHA2.h"
#include "crypto/HMAC-SHA3.h"

namespace librevault {

std::vector<uint8_t> Meta::Chunk::encrypt(const std::vector<uint8_t>& chunk, const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv) {
	return chunk | crypto::AES_CBC(key, iv, chunk.size() % 16 != 0);
}

std::vector<uint8_t> Meta::Chunk::decrypt(const std::vector<uint8_t>& chunk, uint32_t size, const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv) {
	return chunk | crypto::De<crypto::AES_CBC>(key, iv, size % 16 != 0);
}

std::vector<uint8_t> Meta::Chunk::compute_strong_hash(const std::vector<uint8_t>& chunk, StrongHashType type) {
	switch(type){
		case SHA3_224: return chunk | crypto::SHA3(224);
		case SHA2_224: return chunk | crypto::SHA2(224);
		default: return std::vector<uint8_t>();	// TODO: throw some exception.
	}
}

Meta::Meta() {}
Meta::Meta(const std::vector<uint8_t>& meta_s) {
	parse(meta_s);
}
Meta::~Meta() {}

bool Meta::validate() const {
	if(path_id_.size() != 28) return false; // Hash size mismatch
	if(!path_.check()) return false;   // Data/IV size mismatch

	if(meta_type_ == Type::SYMLINK && !symlink_path_.check()) return false;

	if(meta_type_ == Type::FILE) {
		if(algorithm_type_ != RABIN
			|| strong_hash_type_ > SHA2_224 // Unknown cryptographic hashing algorithm
			|| max_chunksize_ == 0 || min_chunksize_ == 0 || max_chunksize_ > min_chunksize_) return false;    // Invalid chunk constraints

		if(algorithm_type_ == RABIN && !rabin_global_params_.check()) return false;

		for(auto& chunk : chunks()) {
			if(chunk.size > max_chunksize_ || chunk.size == 0)   // Broken chunk constraint
			if(chunk.ct_hash.size() != 28) return false;
			if(chunk.iv.size() != 16) return false;

			if(chunk.pt_hmac.size() != 28) return false;
		}
	}

	return true;
}

/*bool Meta::validate(const Secret& secret) const {
	if(! validate()) return false;  // "Easy" validation failed

	if(make_path_id(path(secret), secret) != path_id_) return false;    // path_id is inconsistent with encrypted_path and its iv

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
	for(auto& chunk : chunks()){
		total_size += chunk.size;
	}
	return total_size;
}

std::string Meta::path(const Secret& secret) const {
	std::vector<uint8_t> result = path_.get_plain(secret);
	return std::string(std::make_move_iterator(result.begin()), std::make_move_iterator(result.end()));
}
void Meta::set_path(std::string path, const Secret& secret) {
	set_path_id(make_path_id(path, secret));
	path_.set_plain(std::vector<uint8_t>(std::make_move_iterator(path.begin()), std::make_move_iterator(path.end())), secret);
}

std::string Meta::symlink_path(const Secret& secret) const {
	std::vector<uint8_t> result = symlink_path_.get_plain(secret);
	return std::string(std::make_move_iterator(result.begin()), std::make_move_iterator(result.end()));
}
void Meta::set_symlink_path(std::string path, const Secret& secret) {
	symlink_path_.set_plain(std::vector<uint8_t>(std::make_move_iterator(path.begin()), std::make_move_iterator(path.end())), secret);
}

Meta::RabinGlobalParams Meta::rabin_global_params(const Secret& secret) const {
	auto decrypted = rabin_global_params_.get_plain(secret);

	serialization::Meta::FileMetadata::RabinGlobalParams rabin_global_params_s;
	bool parsed_well = rabin_global_params_s.ParseFromArray(decrypted.data(), decrypted.size());
	if(!parsed_well) throw parse_error("Parse error: Rabin signature parameters parsing failed");

	Meta::RabinGlobalParams rabin_global_params;
	rabin_global_params.polynomial = rabin_global_params_s.polynomial();
	rabin_global_params.polynomial_degree = rabin_global_params_s.polynomial_degree();
	rabin_global_params.polynomial_shift = rabin_global_params_s.polynomial_shift();
	rabin_global_params.avg_bits = rabin_global_params_s.avg_bits();

	return rabin_global_params;
}
void Meta::set_rabin_global_params(const RabinGlobalParams& rabin_global_params, const Secret& secret) {
	serialization::Meta::FileMetadata::RabinGlobalParams rabin_global_params_s;
	rabin_global_params_s.set_polynomial(rabin_global_params.polynomial);
	rabin_global_params_s.set_polynomial_degree(rabin_global_params.polynomial_degree);
	rabin_global_params_s.set_polynomial_shift(rabin_global_params.polynomial_shift);
	rabin_global_params_s.set_avg_bits(rabin_global_params.avg_bits);

	std::vector<uint8_t> serialized(rabin_global_params_s.ByteSize());
	rabin_global_params_s.SerializeToArray(serialized.data(), serialized.size());

	rabin_global_params_.set_plain(serialized, secret);
}

std::vector<uint8_t> Meta::serialize() const {
	serialization::Meta meta_s;

	meta_s.set_path_id(path_id_.data(), path_id_.size());
	meta_s.mutable_path()->set_ct(path_.ct.data(), path_.ct.size());
	meta_s.mutable_path()->set_iv(path_.iv.data(), path_.iv.size());
	meta_s.set_meta_type((uint32_t)meta_type_);
	meta_s.set_revision(revision_);

	if(meta_type_ != DELETED) {
		meta_s.mutable_generic_metadata()->set_mtime(mtime_);
		meta_s.mutable_generic_metadata()->set_windows_attrib(windows_attrib_);
		meta_s.mutable_generic_metadata()->set_mode(mode_);
		meta_s.mutable_generic_metadata()->set_uid(uid_);
		meta_s.mutable_generic_metadata()->set_gid(gid_);
	}

	if(meta_type_ == SYMLINK) {
		meta_s.mutable_symlink_metadata()->mutable_symlink_path()->set_ct(symlink_path_.ct.data(), symlink_path_.ct.size());
		meta_s.mutable_symlink_metadata()->mutable_symlink_path()->set_iv(symlink_path_.iv.data(), symlink_path_.iv.size());
	}

	if(meta_type_ == FILE) {
		meta_s.mutable_file_metadata()->set_algorithm_type((uint32_t)algorithm_type_);
		meta_s.mutable_file_metadata()->set_strong_hash_type((uint32_t)strong_hash_type_);

		meta_s.mutable_file_metadata()->set_max_chunksize(max_chunksize_);
		meta_s.mutable_file_metadata()->set_min_chunksize(min_chunksize_);

		meta_s.mutable_file_metadata()->mutable_rabin_global_params()->set_ct(rabin_global_params_.ct.data(), rabin_global_params_.ct.size());
		meta_s.mutable_file_metadata()->mutable_rabin_global_params()->set_iv(rabin_global_params_.iv.data(), rabin_global_params_.iv.size());

		for(auto& chunk : chunks()) {
			auto chunk_s = meta_s.mutable_file_metadata()->add_chunks();
			chunk_s->set_ct_hash(chunk.ct_hash.data(), chunk.ct_hash.size());
			chunk_s->set_size(chunk.size);
			chunk_s->set_iv(chunk.iv.data(), chunk.iv.size());

			chunk_s->set_pt_hmac(chunk.pt_hmac.data(), chunk.pt_hmac.size());
		}
	}

	std::vector<uint8_t> serialized_data(meta_s.ByteSize());
	meta_s.SerializeToArray(serialized_data.data(), serialized_data.size());
	return serialized_data;
}

void Meta::parse(const std::vector<uint8_t> &serialized_data) {
	serialization::Meta meta_s;

	bool parsed_well = meta_s.ParseFromArray(serialized_data.data(), serialized_data.size());
	if(!parsed_well) throw parse_error("Parse error: Protobuf parsing failed");

	path_id_.assign(meta_s.path_id().begin(), meta_s.path_id().end());
	path_.ct.assign(meta_s.path().ct().begin(), meta_s.path().ct().end());
	path_.iv.assign(meta_s.path().iv().begin(), meta_s.path().iv().end());
	meta_type_ = (Type)meta_s.meta_type();
	revision_ = (int64_t)meta_s.revision();

	if(meta_type_ != DELETED) {
		mtime_ = meta_s.generic_metadata().mtime();
		windows_attrib_ = meta_s.generic_metadata().windows_attrib();
		mode_ = meta_s.generic_metadata().mode();
		uid_ = meta_s.generic_metadata().uid();
		gid_ = meta_s.generic_metadata().gid();
	}else if(meta_s.type_specific_metadata_case() != 0) throw parse_error("Parse error: Redundant type specific metadata found on DELETED");

	if(meta_type_ == SYMLINK) {
		if(meta_s.type_specific_metadata_case() != meta_s.kSymlinkMetadata) throw parse_error("Parse error: Symlink metadata needed");
		symlink_path_.ct.assign(meta_s.symlink_metadata().symlink_path().ct().begin(), meta_s.symlink_metadata().symlink_path().ct().end());
		symlink_path_.iv.assign(meta_s.symlink_metadata().symlink_path().iv().begin(), meta_s.symlink_metadata().symlink_path().iv().end());
	}

	if(meta_type_ == FILE) {
		if(meta_s.type_specific_metadata_case() != meta_s.kFileMetadata) throw parse_error("Parse error: File metadata needed");
		algorithm_type_ = (AlgorithmType)meta_s.file_metadata().algorithm_type();
		strong_hash_type_ = (StrongHashType)meta_s.file_metadata().strong_hash_type();
		max_chunksize_ = meta_s.file_metadata().max_chunksize();
		min_chunksize_ = meta_s.file_metadata().min_chunksize();

		rabin_global_params_.ct.assign(meta_s.file_metadata().rabin_global_params().ct().begin(), meta_s.file_metadata().rabin_global_params().ct().end());
		rabin_global_params_.iv.assign(meta_s.file_metadata().rabin_global_params().iv().begin(), meta_s.file_metadata().rabin_global_params().iv().end());

		for(auto& chunk_s : meta_s.file_metadata().chunks()) {
			Chunk chunk;
			chunk.ct_hash.assign(chunk_s.ct_hash().begin(), chunk_s.ct_hash().end());
			chunk.size = chunk_s.size();
			chunk.iv.assign(chunk_s.iv().begin(), chunk_s.iv().end());
			chunk.pt_hmac.assign(chunk_s.pt_hmac().begin(), chunk_s.pt_hmac().end());

			chunks_.push_back(chunk);
		}
	}
}

std::vector<uint8_t> Meta::make_path_id(const std::string& path, const Secret& secret) {
	return path | crypto::HMAC_SHA3_224(secret.get_Encryption_Key());
}

} /* namespace librevault */
