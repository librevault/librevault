/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include <librevault/Meta.h>
#include <Meta_s.pb.h>

#include <sstream>
#include <librevault/crypto/Base32.h>
#include <librevault/crypto/Hex.h>
#include <librevault/crypto/AES_CBC.h>
#include <osrng.h>

namespace librevault {

Meta::SignedMeta::SignedMeta(blob raw_meta, blob signature, const Key& secret, bool check_signature) :
	raw_meta_(std::make_shared<blob>(std::move(raw_meta))),
	signature_(std::make_shared<blob>(std::move(signature))) {
	// TODO: check signature
	meta_ = std::make_shared<Meta>(*raw_meta_);
}

Meta::Meta() {}
Meta::Meta(const blob& meta_s) {
	parse(meta_s);
}
Meta::~Meta() {}

std::string Meta::debug_string() const {
	std::ostringstream os;

	os	<< "path_id: " << crypto::Base32().to_string(path_id_) << "; "
		<< "encrypted_path: " << crypto::Hex().to_string(encrypted_path_) << "; "
		<< "encrypted_path_iv: " << crypto::Hex().to_string(encrypted_path_iv_) << "; "
		<< "meta_type: " << meta_type_ << "; "
		<< "revision: " << revision_ << "; "

		<< "mtime: " << mtime_ << "; "

		<< "symlink_encrypted_path: " << crypto::Hex().to_string(symlink_encrypted_path_) << "; "
		<< "symlink_encrypted_path_iv: " << crypto::Hex().to_string(symlink_encrypted_path_iv_) << "; "

		<< "windows_attrib: " << windows_attrib_ << "; "
		<< "mode: " << std::oct << mode_ << std::dec << "; "
		<< "uid: " << uid_ << "; "
		<< "gid: " << gid_ << "; "

		<< "max_blocksize: " << max_blocksize_ << "; "
		<< "min_blocksize: " << min_blocksize_ << "; "
		<< "strong_hash_type: " << strong_hash_type_ << "; "
		<< "weak_hash_type: " << weak_hash_type_;
	return os.str();
}

uint64_t Meta::size() const {
	uint64_t total_size = 0;
	for(auto& block : blocks()){
		total_size += block.blocksize_;
	}
	return total_size;
}

cryptodiff::FileMap Meta::filemap(const Key& key) {
	cryptodiff::FileMap new_filemap(key.get_Encryption_Key());

	new_filemap.set_minblocksize(min_blocksize());
	new_filemap.set_maxblocksize(max_blocksize());
	new_filemap.set_strong_hash_type(strong_hash_type());
	new_filemap.set_weak_hash_type(weak_hash_type());
	new_filemap.set_blocks(blocks());

	return new_filemap;
}

void Meta::set_filemap(const cryptodiff::FileMap& new_filemap) {
	set_min_blocksize(new_filemap.minblocksize());
	set_max_blocksize(new_filemap.maxblocksize());
	set_strong_hash_type(new_filemap.strong_hash_type());
	set_weak_hash_type(new_filemap.weak_hash_type());
	set_blocks(new_filemap.blocks());
}

std::string Meta::path(const Key& key) const {
	blob result = encrypted_path_ | crypto::De<crypto::AES_CBC>(key.get_Encryption_Key(), encrypted_path_iv_);
	return std::string(std::make_move_iterator(result.begin()), std::make_move_iterator(result.end()));
}
void Meta::set_path(const std::string& path, const Key& key) {
	randomize_encrypted_path_iv();
	set_encrypted_path(path | crypto::AES_CBC(key.get_Encryption_Key(), encrypted_path_iv_));
}

std::string Meta::symlink_path(const Key& key) const {
	blob result = symlink_encrypted_path_ | crypto::De<crypto::AES_CBC>(key.get_Encryption_Key(), symlink_encrypted_path_iv_);
	return std::string(std::make_move_iterator(result.begin()), std::make_move_iterator(result.end()));
}
void Meta::set_symlink_path(const std::string& path, const Key& key) {
	randomize_symlink_encrypted_path_iv();
	set_symlink_encrypted_path(path | crypto::AES_CBC(key.get_Encryption_Key(), symlink_encrypted_path_iv_));
}

blob Meta::gen_random_iv() const {
	blob random_iv(16);
	CryptoPP::AutoSeededRandomPool rng; rng.GenerateBlock(random_iv.data(), random_iv.size());
	return random_iv;
}

blob Meta::serialize() const {
	serialization::Meta_s meta_s;
	meta_s.set_path_id(path_id().data(), path_id().size());
	meta_s.set_encrypted_path(encrypted_path().data(), encrypted_path().size());
	meta_s.set_encrypted_path_iv(encrypted_path_iv().data(), encrypted_path_iv().size());
	meta_s.set_meta_type((uint32_t)meta_type());
	meta_s.set_revision(revision());

	meta_s.set_mtime(mtime());

	meta_s.set_symlink_encrypted_path(symlink_encrypted_path().data(), symlink_encrypted_path().size());
	meta_s.set_symlink_encrypted_path_iv(symlink_encrypted_path_iv().data(), symlink_encrypted_path_iv().size());

	meta_s.set_windows_attrib(windows_attrib());

	meta_s.set_mode(mode());
	meta_s.set_uid(uid());
	meta_s.set_gid(gid());

	meta_s.set_max_blocksize(max_blocksize());
	meta_s.set_min_blocksize(min_blocksize());
	meta_s.set_strong_hash_type((uint32_t)strong_hash_type());
	meta_s.set_weak_hash_type((uint32_t)weak_hash_type());

	for(auto& block : blocks()){
		auto block_s = meta_s.add_blocks();
		block_s->set_encrypted_data_hash(block.encrypted_data_hash_.data(), block.encrypted_data_hash_.size());
		block_s->set_encrypted_rsync_hashes(block.encrypted_rsync_hashes_.data(), block.encrypted_rsync_hashes_.size());
		block_s->set_blocksize(block.blocksize_);
		block_s->set_iv(block.iv_.data(), block.iv_.size());
	}

	blob serialized_data(meta_s.ByteSize());
	meta_s.SerializeToArray(serialized_data.data(), serialized_data.size());
	return serialized_data;
}

void Meta::parse(const blob &serialized_data) {
	serialization::Meta_s meta_s;

	bool parsed_well = meta_s.ParseFromArray(serialized_data.data(), serialized_data.size());
	if(!parsed_well) throw parse_error();

	path_id_.assign(meta_s.path_id().begin(), meta_s.path_id().end());
	encrypted_path_.assign(meta_s.encrypted_path().begin(), meta_s.encrypted_path().end());
	encrypted_path_iv_.assign(meta_s.encrypted_path_iv().begin(), meta_s.encrypted_path_iv().end());
	meta_type_ = (Type)meta_s.meta_type();
	revision_ = (int64_t)meta_s.revision();

	mtime_ = (int64_t)meta_s.mtime();

	symlink_encrypted_path_.assign(meta_s.symlink_encrypted_path().begin(), meta_s.symlink_encrypted_path().end());
	symlink_encrypted_path_iv_.assign(meta_s.symlink_encrypted_path_iv().begin(), meta_s.symlink_encrypted_path_iv().end());

	windows_attrib_ = meta_s.windows_attrib();

	mode_ = meta_s.mode();
	uid_ = meta_s.uid();
	gid_ = meta_s.gid();

	max_blocksize_ = meta_s.max_blocksize();
	min_blocksize_ = meta_s.min_blocksize();
	strong_hash_type_ = (StrongHashType)meta_s.strong_hash_type();
	weak_hash_type_ = (WeakHashType)meta_s.weak_hash_type();

	for(auto& block_s : meta_s.blocks()){
		Block block;
		block.encrypted_data_hash_.assign(block_s.encrypted_data_hash().begin(), block_s.encrypted_data_hash().end());
		block.encrypted_rsync_hashes_.assign(block_s.encrypted_rsync_hashes().begin(), block_s.encrypted_rsync_hashes().end());
		block.blocksize_ = block_s.blocksize();
		block.iv_.assign(block_s.iv().begin(), block_s.iv().end());
		blocks_.push_back(block);
	}
}

} /* namespace librevault */
