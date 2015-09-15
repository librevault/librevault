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
#include "Indexer.h"
#include "FSDirectory.h"
#include "../../Session.h"
#include "Meta.pb.h"

namespace librevault {

Indexer::Indexer(FSDirectory& dir, Session& session) :
		log_(spdlog::get("Librevault")), dir_(dir),
		key_(dir_.key()), index_(*dir_.index), enc_storage_(*dir_.enc_storage), open_storage_(*dir_.open_storage), session_(session) {}
Indexer::~Indexer() {}

void Indexer::index(const std::string& file_path){
	if(open_storage_.is_skipped(file_path)){
		log_->info() << "Skipping " << file_path;
	}else{
		auto meta = make_Meta(file_path);
		index_.put_Meta(sign(meta));

		Meta m; m.ParseFromArray(meta.data(), meta.size());

		auto path_hmac = m.path_hmac();

		index_.db().exec("UPDATE openfs SET assembled=1 WHERE file_path_hmac=:file_path_hmac", {
				{":file_path_hmac", blob(path_hmac.data(), path_hmac.data()+path_hmac.size())}
		});

		log_->debug() << "Updated index entry. Path=" << file_path << " Rev=" << m.mtime();
	}
}

void Indexer::index(const std::set<std::string>& file_path){
	log_->debug() << "Preparing to index " << file_path.size() << " entries.";
	for(auto file_path1 : file_path)
		index(file_path1);
}

void Indexer::async_index(const std::string& file_path, std::function<void(AbstractDirectory::SignedMeta)> callback) {
	if(open_storage_.is_skipped(file_path)){
		log_->info() << "Skipping " << file_path;
	}else{
		session_.ios().post(std::bind([this](const std::string& file_path, std::function<void(AbstractDirectory::SignedMeta)> callback){
			auto meta = make_Meta(file_path);
			AbstractDirectory::SignedMeta smeta = sign(meta);
			index_.put_Meta(smeta);

			Meta m; m.ParseFromArray(meta.data(), meta.size());

			auto path_hmac = m.path_hmac();

			index_.db().exec("UPDATE openfs SET assembled=1 WHERE file_path_hmac=:file_path_hmac", {
					{":file_path_hmac", blob(path_hmac.data(), path_hmac.data()+path_hmac.size())}
			});

			log_->debug() << "Updated index entry. Path=" << file_path << " Rev=" << m.mtime();

			session_.ios().dispatch(std::bind(callback, smeta));
		}, file_path, callback));
	}
}

void Indexer::async_index(const std::set<std::string>& file_path, std::function<void(AbstractDirectory::SignedMeta)> callback) {
	log_->debug() << "Preparing to index " << file_path.size() << " entries.";
	for(auto file_path1 : file_path)
		async_index(file_path1, callback);
}

blob Indexer::make_Meta(const std::string& relpath){
	Meta meta;
	auto abspath = fs::absolute(relpath, dir_.open_path());

	// Path_HMAC
	blob path_hmac = open_storage_.make_path_hmac(relpath);
	meta.set_path_hmac(path_hmac.data(), path_hmac.size());

	// EncPath_IV
	blob encpath_iv(16);
	CryptoPP::AutoSeededRandomPool rng; rng.GenerateBlock(encpath_iv.data(), encpath_iv.size());
	meta.set_encpath_iv(encpath_iv.data(), encpath_iv.size());

	// EncPath
	auto encpath = crypto::AES_CBC(key_.get_Encryption_Key(), encpath_iv).encrypt(relpath);
	meta.set_encpath(encpath.data(), encpath.size());

	// Type
	auto file_status = fs::symlink_status(abspath);
	switch(file_status.type()){
	case fs::regular_file: {
		// Add FileMeta for files
		meta.set_type(Meta::FileType::Meta_FileType_FILE);

		cryptodiff::FileMap filemap(crypto::BinaryArray(key_.get_Encryption_Key()));
		try {
			auto old_smeta = index_.get_Meta(path_hmac);
			Meta old_meta; old_meta.ParseFromArray(old_smeta.meta.data(), old_smeta.meta.size());	// Trying to retrieve Meta from database. May throw.
			//auto map = old_meta.filemap();
			filemap.from_string(old_meta.filemap().SerializeAsString());
			filemap.update(abspath.native());
		}catch(Index::no_such_meta& e){
			filemap.create(abspath.native());
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
#if BOOST_OS_WINDOWS
		meta.set_windows_attrib(GetFileAttributes(relpath.native().c_str()));	// Windows attributes (I don't have Windows now to test it)
#endif
	}else{
		meta.set_mtime(std::time(nullptr));	// mtime
	}

	blob result_meta(meta.ByteSize()); meta.SerializeToArray(result_meta.data(), result_meta.size());
	return result_meta;
}

AbstractDirectory::SignedMeta Indexer::sign(const blob& meta) const {
	CryptoPP::AutoSeededRandomPool rng;
	AbstractDirectory::SignedMeta result;
	result.meta = meta;

	CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Signer signer;
	signer.AccessKey().Initialize(CryptoPP::ASN1::secp256r1(), CryptoPP::Integer(key_.get_Private_Key().data(), key_.get_Private_Key().size()));

	result.signature.resize(signer.SignatureLength());
	signer.SignMessage(rng, meta.data(), meta.size(), result.signature.data());
	return result;
}

} /* namespace librevault */
