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
#include <Meta.pb.h>

#include "../../contrib/lvsqlite3/SQLiteWrapper.h"
#include "../../contrib/crypto/AES_CBC.h"
#include "../../contrib/crypto/Base32.h"
#include "../../contrib/crypto/HMAC-SHA3.h"
#include <cryptopp/oids.h>

#include "../../contrib/crypto/Base64.h"

// Cryptodiff
#include <cryptodiff.h>

// CryptoPP
#include <cryptopp/osrng.h>

// Boost
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>	// It is not necessary. But my Eclipse craps me out if I don't include it.

namespace librevault {
namespace syncfs {

SyncFS::SyncFS(boost::asio::io_service& io_service, Key key, fs::path open_path, fs::path block_path, fs::path db_path) :
		external_io_service(io_service),
		key(std::move(key)),
		open_path(std::move(open_path)),
		db_path(std::move(db_path)),
		block_path(std::move(block_path)),
		internal_io_service(std::shared_ptr<boost::asio::io_service>(new boost::asio::io_service())),
		internal_io_strand(*internal_io_service),
		external_io_strand(external_io_service){
	BOOST_LOG_TRIVIAL(debug) << "Initializing SYNCFS";
	BOOST_LOG_TRIVIAL(debug) << "Key level " << (char)key.getType();

	BOOST_LOG_TRIVIAL(debug) << "Open directory: " << this->open_path << (fs::create_directories(this->open_path) ? " created" : "");
	BOOST_LOG_TRIVIAL(debug) << "Block directory: " << this->block_path << (fs::create_directories(this->block_path) ? " created" : "");
	BOOST_LOG_TRIVIAL(debug) << "Database directory: " << this->db_path.parent_path() << (fs::create_directories(this->db_path.parent_path()) ? " created" : "");

	if(fs::exists(this->db_path))
		BOOST_LOG_TRIVIAL(debug) << "Opening SQLite3 DB: " << this->db_path;
	else
		BOOST_LOG_TRIVIAL(debug) << "Creating new SQLite3 DB: " << this->db_path;
	directory_db = std::make_shared<SQLiteDB>(this->db_path);
	directory_db->exec("PRAGMA foreign_keys = ON;");

	directory_db->exec("CREATE TABLE IF NOT EXISTS files (path_hmac BLOB PRIMARY KEY NOT NULL, mtime INTEGER NOT NULL, meta BLOB NOT NULL, signature BLOB NOT NULL);");
	directory_db->exec("CREATE TABLE IF NOT EXISTS blocks (encrypted_hash BLOB NOT NULL PRIMARY KEY, blocksize INTEGER NOT NULL, iv BLOB NOT NULL);");
	directory_db->exec("CREATE TABLE IF NOT EXISTS openfs (block_encrypted_hash BLOB NOT NULL REFERENCES blocks (encrypted_hash) ON DELETE CASCADE ON UPDATE CASCADE, file_path_hmac BLOB NOT NULL REFERENCES files (path_hmac) ON DELETE CASCADE ON UPDATE CASCADE, [offset] INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");

	//directory_db->exec("CREATE TRIGGER IF NOT EXISTS block_deleter AFTER DELETE ON files BEGIN DELETE FROM blocks WHERE id NOT IN (SELECT blockid FROM openfs); END;");

	enc_storage = std::unique_ptr<EncStorage>(new EncStorage(this->block_path));
	open_storage = std::unique_ptr<OpenStorage>(new OpenStorage(this, this->key, directory_db, *enc_storage.get(), this->open_path, this->block_path));
}

SyncFS::~SyncFS() {}

blob SyncFS::make_Meta(const std::string& file_path){
	Meta meta;
	auto abspath = fs::absolute(file_path, open_path);

	// Path_HMAC
	blob path_hmac = crypto::HMAC_SHA3_224(key.get_Encryption_Key()).to(file_path);
	meta.set_path_hmac(path_hmac.data(), path_hmac.size());

	// EncPath_IV
	blob encpath_iv(16);
	CryptoPP::AutoSeededRandomPool rng; rng.GenerateBlock(encpath_iv.data(), encpath_iv.size());
	meta.set_encpath_iv(encpath_iv.data(), encpath_iv.size());

	// EncPath
	auto encpath = crypto::AES_CBC(key.get_Encryption_Key(), encpath_iv).encrypt(file_path);
	meta.set_encpath(encpath.data(), encpath.size());

	// Type
	auto file_status = fs::symlink_status(abspath);
	switch(file_status.type()){
	case fs::regular_file: {
		// Add FileMeta for files
		meta.set_type(Meta::FileType::Meta_FileType_FILE);

		cryptodiff::FileMap filemap(crypto::BinaryArray(key.get_Encryption_Key()));
		fs::ifstream ifs(abspath, std::ios_base::binary);
		try {
			auto old_smeta = get_Meta(path_hmac);
			Meta old_meta; old_meta.ParseFromArray(old_smeta.meta.data(), old_smeta.meta.size());	// Trying to retrieve Meta from database. May throw.
			//auto map = old_meta.filemap();
			filemap.from_string(old_meta.filemap().SerializeAsString());
			filemap.update(ifs);
		}catch(no_such_meta& e){
			filemap.create(ifs);
		}
		meta.mutable_filemap()->ParseFromString(filemap.to_string());	// TODO: Amazingly dirty.
	} break;
	case fs::directory_file: {
		meta.set_type(Meta::FileType::Meta_FileType_DIRECTORY);
	} break;
	case fs::symlink_file: {
		meta.set_type(Meta::FileType::Meta_FileType_SYMLINK);
		meta.set_symlink_to(fs::read_symlink(abspath).generic_string());	// TODO: Make it optional #symlink
	} break;
	case fs::file_not_found: {
		meta.set_type(Meta::FileType::Meta_FileType_DELETED);
	} break;
	}

	if(meta.type() != Meta::FileType::Meta_FileType_DELETED){
		meta.set_mtime(fs::last_write_time(abspath));	// mtime
#ifdef _WIN32
		meta.set_windows_attrib(GetFileAttributes(relpath.native().c_str()));	// Windows attributes (I don't have Windows now to test it)
#endif
	}else{
		meta.set_mtime(std::time(nullptr));	// mtime
	}

	blob result_meta(meta.ByteSize()); meta.SerializeToArray(result_meta.data(), result_meta.size());
	return result_meta;
}

SignedMeta SyncFS::sign(const blob& meta) const {
	CryptoPP::AutoSeededRandomPool rng;
	SignedMeta result;
	result.meta = meta;

	CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Signer signer;
	signer.AccessKey().Initialize(CryptoPP::ASN1::secp256r1(), CryptoPP::Integer(key.get_Private_Key().data(), key.get_Private_Key().size()));

	result.signature.resize(signer.SignatureLength());
	signer.SignMessage(rng, meta.data(), meta.size(), result.signature.data());
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

void SyncFS::index(const std::set<std::string> file_path){
	BOOST_LOG_TRIVIAL(debug) << "Preparing to index " << file_path.size() << " entries.";
	for(auto file_path1 : file_path){
		internal_io_strand.post(std::bind([this](std::string file_path_bound){	// Can be executed in any thread.
			put_Meta(sign(make_Meta(file_path_bound)));

			auto path_hmac = crypto::HMAC_SHA3_224(key.get_Encryption_Key()).to(file_path_bound);

			directory_db->exec("UPDATE openfs SET assembled=1 WHERE file_path_hmac=:file_path_hmac", {
					{":file_path_hmac", blob(path_hmac.data(), path_hmac.data()+path_hmac.size())}
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
		if(dir_entry_it->path() == db_path) continue;
		if(dir_entry_it->path() == block_path) continue;

		std::string block_path_canonical = fs::canonical(block_path).generic_string();
		if(block_path_canonical == fs::canonical(dir_entry_it->path(), open_path).string().substr(0, block_path_canonical.size())) continue;
		// Prevents system directories from scan. TODO skiplist

		file_list.insert(make_portable_path(dir_entry_it->path()));
	}
	return file_list;
}

std::set<std::string> SyncFS::get_index_file_list(){
	std::set<std::string> file_list;

	for(auto row : directory_db->exec("SELECT meta FROM files")){
		Meta meta; meta.ParseFromString(row[0].as_text());
		file_list.insert(crypto::AES_CBC(key.get_Encryption_Key(), meta.encpath_iv()).from(meta.encpath()));
	}

	return file_list;
}

void SyncFS::put_Meta(std::list<SignedMeta> signed_meta_list) {
	auto raii_lock = SQLiteLock(directory_db.get());
	for(auto signed_meta : signed_meta_list){
		SQLiteSavepoint raii_transaction(directory_db.get(), "put_Meta");
		Meta meta; meta.ParseFromArray(signed_meta.meta.data(), signed_meta.meta.size());

		auto file_path_hmac = SQLValue((const uint8_t*)meta.path_hmac().data(), meta.path_hmac().size());
		directory_db->exec("INSERT OR REPLACE INTO files (path_hmac, mtime, meta, signature) VALUES (:path_hmac, :mtime, :meta, :signature);", {
				{":path_hmac", file_path_hmac},
				{":mtime", meta.mtime()},
				{":meta", SQLValue((uint8_t*)signed_meta.meta.data(), signed_meta.meta.size())},
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
	}
}

std::list<SignedMeta> SyncFS::get_Meta(std::string sql, std::map<std::string, SQLValue> values){
	std::list<SignedMeta> result_list;
	for(auto row : directory_db->exec(sql, values))
		result_list.push_back({row[0], row[1]});
	return result_list;
}
SignedMeta SyncFS::get_Meta(blob path_hmac){
	auto meta_list = get_Meta("SELECT meta, signature FROM files WHERE path_hmac=:path_hmac LIMIT 1", {
			{":path_hmac", path_hmac}
	});

	if(meta_list.empty()) throw no_such_meta();
	return *meta_list.begin();
}
std::list<SignedMeta> SyncFS::get_Meta(int64_t mtime){
	return get_Meta("SELECT meta, signature FROM files WHERE mtime >= :mtime", {{":mtime", mtime}});
}
std::list<SignedMeta> SyncFS::get_Meta(){
	return get_Meta("SELECT meta, signature FROM files");
}

blob SyncFS::get_block(const blob& block_hash){
	try {
		for(auto row : directory_db->exec("SELECT iv FROM blocks WHERE encrypted_hash=:encrypted_hash", {{":encrypted_hash", block_hash}})){
			return crypto::AES_CBC(key.get_Encryption_Key(), row[0].as_blob(), true).from(enc_storage->get_encblock(block_hash));
		}
	}catch(no_such_block& e){
		return open_storage->get_block(block_hash);
	}
}
void SyncFS::put_block(const blob& block_hash, const blob& data){
	for(auto row : directory_db->exec("SELECT iv FROM blocks WHERE encrypted_hash=:encrypted_hash", {{":encrypted_hash", block_hash}})){
		enc_storage->put_encblock(block_hash, crypto::AES_CBC(key.get_Encryption_Key(), row[0].as_blob(), true).to(data));
	}
}

blob SyncFS::get_encblock(const blob& block_hash){
	try {
		return enc_storage->get_encblock(block_hash);
	}catch(no_such_block& e){
		return open_storage->get_encblock(block_hash);
	}
}
void SyncFS::put_encblock(const blob& block_hash, const blob& data){
	enc_storage->put_encblock(block_hash, data);
}

} /* namespace syncfs */
} /* namespace librevault */
