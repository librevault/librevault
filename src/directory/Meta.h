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

/*
message Meta_s {
	// These I'd mark as required
	bytes path_hmac = 1;
	bytes encpath = 2;	// AES-256-CBC encrypted normalized path relative to sync directory path. It cannot contain dot (".") and dot-dot ("..") elements. Directory path MUST NOT end with trailing slash ("/"). Separators MUST be slashes ("/").
	bytes encpath_iv = 3;	// IV for AES-256-CBC encrypted encpath.

	enum FileType {
		FILE = 0;
		DIRECTORY = 1;
		SYMLINK = 2;

		DELETED = 255;
	}
	FileType type = 4;
	int64 mtime = 5;

	// These are optional (I mean, conditional)
	bytes symlink_to = 6;
	uint32 windows_attrib = 7;

	cryptodiff.internals.EncFileMap_s filemap = 16;
}
*/

class Meta {
public:
	enum Type : uint8_t {FILE = 0, DIRECTORY = 1, SYMLINK = 2, DELETED = 255};
	using StrongHashType = cryptodiff::StrongHashType;
	using WeakHashType = cryptodiff::WeakHashType;
	using Block = cryptodiff::Block;
private:
	/* Meta fields, must be serialized together and then signed */
	blob path_id_;	// aka path_id;
	blob encrypted_path_;	// aka encpath;
	blob encrypted_path_iv_;	// aka encpath_iv;
	Type meta_type_;
	int64_t revision_;	// aka mtime

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
	uint32_t max_blocksize_;
	uint32_t min_blocksize_;
	StrongHashType strong_hash_type_ = StrongHashType::SHA3_224;
	WeakHashType weak_hash_type_ = WeakHashType::RSYNC;

	std::vector<Block> blocks_;

public:
	struct PathRevision {
		blob path_id_;
		int64_t revision_;
	};

	Meta();
	virtual ~Meta();

	blob serialize() const;
	void parse(const blob& serialized_data);

	PathRevision gen_path_revision() const {
		return PathRevision{path_id(), revision()};
	}

	std::string gen_relpath(const Key& key);

	// Getters & setters
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

	const blob& symlink_encrypted_path() const {return symlink_encrypted_path_;}
	void set_symlink_encrypted_path(const blob& symlink_encrypted_path) {symlink_encrypted_path_ = symlink_encrypted_path;}

	const blob& symlink_encrypted_path_iv() const {return symlink_encrypted_path_iv_;}
	void set_symlink_encrypted_path_iv(const blob& symlink_encrypted_path_iv) {symlink_encrypted_path_iv_ = symlink_encrypted_path_iv;}

	uint32_t windows_attrib() const {return windows_attrib_;}
	void set_windows_attrib(uint32_t windows_attrib) {windows_attrib_ = windows_attrib;}

	uint32_t min_blocksize() const {return min_blocksize_;}
	void set_min_blocksize(uint32_t min_blocksize) {min_blocksize_ = min_blocksize;}

	uint32_t max_blocksize() const {return max_blocksize_;}
	void set_max_blocksize(uint32_t max_blocksize) {max_blocksize_ = max_blocksize;}

	HashType hash_type() const {return hash_type_;}
	void set_hash_type(HashType hash_type) {hash_type_ = hash_type;}

	const std::vector<Block>& blocks() const {return blocks_;}
	void set_blocks(const std::vector<Block>& blocks) {blocks_ = blocks;}
};

} /* namespace librevault */
