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
#include "SyncFS.h"
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>	// It is not necessary. But my Eclipse craps me out if I don't include it.

namespace librevault {
namespace syncfs {

SyncFS::SyncFS(boost::asio::io_service& io_service, Key key, fs::path open_path, fs::path db_path, fs::path block_path) :
				external_io_service(io_service),
				key(std::move(key)),
				open_path(std::move(open_path)),
				db_path(std::move(db_path)),
				block_path(std::move(block_path)),
				internal_io_service(std::shared_ptr(new boost::asio::io_service())),
				external_io_strand(external_io_service) {
	// Creating necessary directories
	BOOST_LOG_TRIVIAL(debug) << "Setting OPENFS directory: " << open_path;
	if(fs::create_directories(open_path))	BOOST_LOG_TRIVIAL(debug) << "Setting OPENFS directory: " << open_path;

	directory_db = std::make_shared<SQLiteDB>(this->db_path);
	directory_db->exec("PRAGMA foreign_keys = ON;");

	directory_db->exec("CREATE TABLE IF NOT EXISTS files (id INTEGER PRIMARY KEY, path STRING UNIQUE, path_hmac BLOB NOT NULL UNIQUE, mtime INTEGER NOT NULL, meta BLOB NOT NULL, signature BLOB NOT NULL);");
	directory_db->exec("CREATE TABLE IF NOT EXISTS blocks (id INTEGER PRIMARY KEY, encrypted_hash BLOB NOT NULL UNIQUE, blocksize INTEGER NOT NULL, iv BLOB NOT NULL, in_encfs BOOLEAN NOT NULL DEFAULT (0));");
	directory_db->exec("CREATE TABLE IF NOT EXISTS openfs (blockid INTEGER REFERENCES blocks (id) ON DELETE CASCADE ON UPDATE CASCADE NOT NULL, fileid INTEGER REFERENCES files (id) ON DELETE CASCADE ON UPDATE CASCADE NOT NULL, [offset] INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");
	directory_db->exec("CREATE VIEW IF NOT EXISTS block_presence AS SELECT DISTINCT id, CASE WHEN blocks.in_encfs = 1 OR openfs.assembled = 1 THEN 1 ELSE 0 END AS present, blocks.in_encfs AS in_encfs, openfs.assembled AS in_openfs FROM blocks LEFT JOIN openfs ON blocks.id = openfs.blockid;");

	directory_db->exec("CREATE TRIGGER IF NOT EXISTS block_deleter AFTER DELETE ON files BEGIN DELETE FROM blocks WHERE id NOT IN (SELECT blockid FROM openfs); END;");
}

SyncFS::~SyncFS() {}

std::string SyncFS::make_Meta(const std::string& file_path){
	Meta meta;
	auto abspath = fs::absolute(file_path, open_path);

	// Path_HMAC
	blob path_hmac = crypto::HMAC_SHA3_224(key.get_Encryption_Key()).to(file_path);
	meta.set_path_hmac(path_hmac.data(), path_hmac.size());

	// Type
	auto file_status = fs::symlink_status(abspath);
	switch(file_status.type()){
	case fs::regular_file: {
		// Add FileMeta for files
		meta.set_type(Meta::FileType::Meta_FileType_FILE);

		cryptodiff::FileMap filemap(crypto::BinaryArray());
		fs::ifstream ifs(abspath, std::ios_base::binary);
		try {
			Meta old_meta; old_meta.ParseFromString(get_Meta(path_hmac).meta);
			filemap.from_string(old_meta.filemap().SerializeAsString());	// Trying to retrieve Meta from database. May throw.
			filemap.update(ifs);
		}catch(no_such_meta& e){
			filemap.create(ifs);
		}
		meta.mutable_filemap()->ParseFromString(filemap.to_string());	// TODO: Amazingly dirty.
	} break;
	case fs::directory_file:
		meta.set_type(Meta::FileType::Meta_FileType_DIRECTORY); break;
	case fs::symlink_file:
		meta.set_type(Meta::FileType::Meta_FileType_SYMLINK);
		meta.set_symlink_to(fs::read_symlink(abspath).generic_string());	// TODO: Make it optional #symlink
		break;
	case fs::file_not_found:
		meta.set_type(Meta::FileType::Meta_FileType_DELETED);
		break;
	}

	// EncPath_IV
	crypto::BinaryArray encpath_iv(16);
	CryptoPP::AutoSeededRandomPool rng; rng.GenerateBlock(encpath_iv.data(), encpath_iv.size());
	meta.set_encpath_iv((std::string)encpath_iv);

	// EncPath
	auto encpath = crypto::AES_CBC(key.get_Encryption_Key(), encpath_iv).encrypt(file_path);
	meta.set_encpath(encpath.data(), encpath.size());

	if(meta.type() != Meta::FileType::Meta_FileType_DELETED){
		meta.set_mtime(fs::last_write_time(abspath));	// mtime
#ifdef _WIN32
		meta.set_windows_attrib(GetFileAttributes(relpath.native().c_str()));	// Windows attributes (I don't have Windows now to test it)
#endif
	}else{
		meta.set_mtime(std::time(nullptr));	// mtime
	}

	return meta.SerializeAsString();
}

SignedMeta SyncFS::sign(std::string meta) const {
	CryptoPP::AutoSeededRandomPool rng;
	SignedMeta result;

	auto private_key_v = key.get_Private_Key();
	CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Signer signer;
	signer.AccessKey().SetPrivateExponent(CryptoPP::Integer(private_key_v.data(), private_key_v.size()));

	result.signature.resize(signer.SignatureLength());

	signer.SignMessage(rng, (uint8_t*)meta.data(), meta.size(), result.signature.data());
	return result;
}

std::string SyncFS::make_portable_path(const fs::path& path) const{
	fs::path rel_to = open_path;
	auto abspath = fs::absolute(path);

	fs::path relpath;
	auto path_elem_it = abspath.begin();
	for(auto dir_elem : rel_to){
		if(dir_elem != *(path_elem_it++))
			return "";
	}
	for(; path_elem_it != abspath.end(); path_elem_it++){
		if(*path_elem_it == "." || *path_elem_it == "..")
			return "";
		relpath /= *path_elem_it;
	}
	return relpath.generic_string();
}

void SyncFS::queue_index(const std::set<std::string> file_path){
	for(auto file_path1 : file_path){
		internal_io_strand.post(std::bind([this](std::string file_path_bound){	// Can be executed in any thread.
			put_Meta(sign(make_Meta(file_path_bound)));
			directory_db->exec("UPDATE openfs SET assembled=1 WHERE fileid IN (SELECT id FROM files WHERE path=:path)", {
					{":path", file_path_bound}
			});

			BOOST_LOG_TRIVIAL(debug) << "Updated index entry. Path=" << file_path_bound;
		}, file_path1));

		external_io_strand.post(
				std::bind((size_t(boost::asio::io_service::*)()) &boost::asio::io_service::poll_one, internal_io_service)	// I hate getting pointers to overloaded methods!
		);
	}
}

std::set<std::string> SyncFS::get_openfs_file_list(){
	std::set<std::string> file_list;
	for(auto dir_entry_it = fs::recursive_directory_iterator(open_path); dir_entry_it != fs::recursive_directory_iterator(); dir_entry_it++){
		if(dir_entry_it->path() != block_path && dir_entry_it->path() != db_path)	// Prevents system directories from scan. TODO skiplist
			file_list.insert(make_portable_path(dir_entry_it->path()));
	}
	return file_list;
}

void SyncFS::put_Meta(SignedMeta signed_meta) {
	internal_io_strand.post(std::bind(
	[this](SignedMeta signed_meta){
		SQLiteSavepoint raii_savepoint(directory_db, "put_Meta");
		Meta meta; meta.ParseFromString(signed_meta.meta);

		auto file_path_hmac = SQLValue((const uint8_t*)meta.path_hmac().data(), meta.path_hmac().size());
		directory_db->exec("INSERT OR REPLACE INTO files (path_hmac, mtime, meta, signature) VALUES (:path_hmac, :mtime, :meta, :signature);", {
				{":path_hmac", file_path_hmac},
				{":mtime", meta.mtime()},
				{":meta", signed_meta.meta},
				{":signature", signed_meta.signature}
		});

		uint64_t offset = 0;
		for(auto block : meta.filemap().blocks()){
			auto block_encrypted_hash = SQLValue((const uint8_t*)block.encrypted_hash().data(), block.encrypted_hash().size());
			directory_db->exec("INSERT OR IGNORE INTO blocks (encrypted_hash, blocksize, iv) VALUES (:encrypted_hash, :blocksize, :iv);", {
					{":encrypted_hash", block_encrypted_hash},
					{":blocksize", (int64_t)block.blocksize()},
					{":iv", SQLValue((const uint8_t*)block.iv().data(), block.iv().size())}
			});

			directory_db->exec("INSERT INTO openfs (block_encrypted_hash, file_path_hmac, [offset]) VALUES (:block_encrypted_hash, :file_path_hmac, :offset);", {
					{":block_encrypted_hash", block_encrypted_hash},
					{":file_path_hmac", file_path_hmac},
					{":offset", (int64_t)offset}
			});

			offset += block.blocksize();
		}

		BOOST_LOG_TRIVIAL(debug) << "Added Meta of " << (std::string)crypto::Base32().to(meta.path_hmac());
	}, signed_meta));

	external_io_strand.dispatch(std::bind((size_t(boost::asio::io_service::*)()) &boost::asio::io_service::poll_one, internal_io_service));
}

} /* namespace syncfs */
} /* namespace librevault */
