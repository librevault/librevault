/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "../pch.h"
#include "Key.h"

namespace librevault {

class Meta {
public:
	enum Type : uint8_t {FILE = 0, DIRECTORY = 1, SYMLINK = 2, DELETED = 255};
	using StrongHashType = cryptodiff::StrongHashType;
	using WeakHashType = cryptodiff::WeakHashType;
	using Block = cryptodiff::Block;
private:
	/* Meta fields, must be serialized together and then signed */
	blob path_id_;	// aka path_hmac;
	blob encrypted_path_;	// aka encpath;
	blob encrypted_path_iv_;	// aka encpath_iv;
	Type meta_type_ = FILE;
	int64_t revision_ = 0;	// timestamp of Meta modification

	int64_t mtime_ = 0;	// file/directory mtime

	// Symlinks only
	blob symlink_encrypted_path_;
	blob symlink_encrypted_path_iv_;

	// Windows-specific
	uint32_t windows_attrib_ = 0;
	// Unix-specific
	uint32_t mode_ = 0;
	uint32_t uid_ = 0;
	uint32_t gid_ = 0;

	// Filemap-derived data
	uint32_t max_blocksize_ = 0;
	uint32_t min_blocksize_ = 0;
	StrongHashType strong_hash_type_ = StrongHashType::SHA3_224;
	WeakHashType weak_hash_type_ = WeakHashType::RSYNC;

	std::vector<Block> blocks_;

public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("Meta error") {}
	};

	struct parse_error : error {
		parse_error() : error("Parse error") {}
	};

	struct PathRevision {
		blob path_id_;
		int64_t revision_;
	};

	struct SignedMeta {
		blob meta_;
		blob signature_;
	};

	Meta();
	Meta(const blob& meta_s);
	virtual ~Meta();

	/* Serialization */
	blob serialize() const;
	void parse(const blob& serialized_data);

	/* Debug */
	std::string debug_string() const;

	/* Smart getters+setters */
	PathRevision path_revision() const {
		return PathRevision{path_id(), revision()};
	}
	uint64_t size() const;

	cryptodiff::FileMap filemap(const Key& key);
	void set_filemap(const cryptodiff::FileMap& new_filemap);

	// Path encryptors+setters
	std::string path(const Key& key) const;
	void set_path(const std::string& path, const Key& key);

	std::string symlink_path(const Key& key) const;
	void set_symlink_path(const std::string& path, const Key& key);

	// IV randomizers
	void randomize_encrypted_path_iv() {set_encrypted_path_iv(gen_random_iv());}
	void randomize_symlink_encrypted_path_iv() {set_symlink_encrypted_path_iv(gen_random_iv());}

	// Dumb getters & setters
	const blob& path_id() const {return path_id_;}
	void set_path_id(const blob& path_id) {path_id_ = path_id;}

	const blob& encrypted_path() const {return encrypted_path_;}
	void set_encrypted_path(const blob& encrypted_path) {encrypted_path_ = encrypted_path;}

	const blob& encrypted_path_iv() const {return encrypted_path_iv_;}
	void set_encrypted_path_iv(const blob& encrypted_path_iv) {encrypted_path_iv_ = encrypted_path_iv;}

	Type meta_type() const {return meta_type_;}
	void set_meta_type(Type meta_type) {meta_type_ = meta_type;}

	int64_t revision() const {return revision_;}
	void set_revision(int64_t revision) {revision_ = revision;}

	int64_t mtime() const {return mtime_;}
	void set_mtime(int64_t mtime) {mtime_ = mtime;}

	const blob& symlink_encrypted_path() const {return symlink_encrypted_path_;}
	void set_symlink_encrypted_path(const blob& symlink_encrypted_path) {symlink_encrypted_path_ = symlink_encrypted_path;}

	const blob& symlink_encrypted_path_iv() const {return symlink_encrypted_path_iv_;}
	void set_symlink_encrypted_path_iv(const blob& symlink_encrypted_path_iv) {symlink_encrypted_path_iv_ = symlink_encrypted_path_iv;}

	uint32_t windows_attrib() const {return windows_attrib_;}
	void set_windows_attrib(uint32_t windows_attrib) {windows_attrib_ = windows_attrib;}

	uint32_t mode() const {return mode_;}
	void set_mode(uint32_t mode) {mode_ = mode;}

	uint32_t uid() const {return uid_;}
	void set_uid(uint32_t uid) {uid_ = uid;}

	uint32_t gid() const {return gid_;}
	void set_gid(uint32_t gid) {gid_ = gid;}

	uint32_t min_blocksize() const {return min_blocksize_;}
	void set_min_blocksize(uint32_t min_blocksize) {min_blocksize_ = min_blocksize;}

	uint32_t max_blocksize() const {return max_blocksize_;}
	void set_max_blocksize(uint32_t max_blocksize) {max_blocksize_ = max_blocksize;}

	StrongHashType strong_hash_type() const {return strong_hash_type_;}
	void set_strong_hash_type(StrongHashType strong_hash_type) {strong_hash_type_ = strong_hash_type;}

	WeakHashType weak_hash_type() const {return weak_hash_type_;}
	void set_weak_hash_type(WeakHashType weak_hash_type) {weak_hash_type_ = weak_hash_type;}

	const std::vector<Block>& blocks() const {return blocks_;}
	void set_blocks(const std::vector<Block>& blocks) {blocks_ = blocks;}
private:
	blob gen_random_iv() const;
};

} /* namespace librevault */
