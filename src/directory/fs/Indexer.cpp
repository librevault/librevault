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
#include "../../util/byte_convert.h"

namespace librevault {

Indexer::Indexer(FSDirectory& dir, Session& session) :
		log_(spdlog::get("Librevault")), dir_(dir),
		key_(dir_.key()), index_(*dir_.index), enc_storage_(*dir_.enc_storage), open_storage_(*dir_.open_storage), session_(session) {}
Indexer::~Indexer() {}

Meta::SignedMeta Indexer::index(const std::string& file_path){
	if(dir_.ignore_list->is_ignored(file_path)){
		log_->notice() << dir_.log_tag() << "Skipping " << file_path;
		return Meta::SignedMeta();
	}else
		try{
			std::chrono::steady_clock::time_point before_index = std::chrono::steady_clock::now();

			auto meta = make_Meta(file_path);
			Meta::SignedMeta smeta = sign(meta);
			index_.put_Meta(sign(meta));

			std::chrono::steady_clock::time_point after_index = std::chrono::steady_clock::now();
			float time_spent = std::chrono::duration<float, std::chrono::seconds::period>(after_index - before_index).count();

			index_.db().exec("UPDATE openfs SET assembled=1 WHERE file_path_hmac=:file_path_hmac", {
					{":file_path_hmac", meta.path_id()}
			});

			log_->trace() << dir_.log_tag() << meta.debug_string();
			log_->debug() << dir_.log_tag() << "Updated index entry in " << time_spent << "s (" << size_to_string((double)meta.size()/time_spent) << "/s)" << ". Path=" << file_path << " Rev=" << meta.revision();

			return smeta;
		}catch(std::runtime_error& e){
			log_->notice() << dir_.log_tag() << "Skipping " << file_path << ". Reason: " << e.what();
			return Meta::SignedMeta();
		}
}

void Indexer::index(const std::set<std::string>& file_path){
	log_->debug() << dir_.log_tag() << "Preparing to index " << file_path.size() << " entries.";
	for(auto file_path1 : file_path)
		index(file_path1);
}

void Indexer::async_index(const std::string& file_path, std::function<void(Meta::SignedMeta)> callback) {
	session_.ios().post(std::bind([this](const std::string& file_path, std::function<void(Meta::SignedMeta)> callback){

		auto smeta = index(file_path);
		session_.ios().dispatch(std::bind(callback, smeta));
	}, file_path, callback));
}

void Indexer::async_index(const std::set<std::string>& file_path, std::function<void(Meta::SignedMeta)> callback) {
	log_->debug() << dir_.log_tag() << "Preparing to index " << file_path.size() << " entries.";
	for(auto file_path1 : file_path)
		async_index(file_path1, callback);
}

Meta Indexer::make_Meta(const std::string& relpath){
	Meta meta;
	auto abspath = fs::absolute(relpath, dir_.open_path());

	// Path ID aka HMAC of relpath aka SHA3(key | relpath);
	meta.set_path_id(open_storage_.make_path_id(relpath));
	// Encrypted path with IV
	meta.set_path(relpath, key_);

	// Type
	fs::file_status file_status = dir_.dir_options().get("preserve_symlinks", true) ? fs::symlink_status(abspath) : fs::status(abspath);	// Preserves symlinks if such option is set.

	switch(file_status.type()){
		case fs::regular_file: meta.set_meta_type(Meta::FILE); break;
		case fs::directory_file: meta.set_meta_type(Meta::DIRECTORY); break;
		case fs::symlink_file: meta.set_meta_type(Meta::SYMLINK); break;
		case fs::file_not_found: meta.set_meta_type(Meta::DELETED); break;
		default: throw unsupported_filetype();
	}

	if(meta.meta_type() != Meta::DELETED){
		try {	// Tries to get old Meta from index. May throw if no such meta or if Meta is invalid (parsing failed).
			Meta old_meta(index_.get_Meta(meta.path_id()).meta_);

			// Preserve old values of attributes
			meta.set_windows_attrib(old_meta.windows_attrib());
			meta.set_mode(old_meta.mode());
			meta.set_uid(old_meta.uid());
			meta.set_gid(old_meta.gid());

			if(meta.meta_type() == Meta::FILE){
				// Update FileMap
				cryptodiff::FileMap filemap = old_meta.filemap(key_);
				filemap.update(abspath.generic_string());
				meta.set_filemap(filemap);
			}
		}catch(...){
			if(meta.meta_type() == Meta::FILE){
				// Create FileMap, if something got wrong
				cryptodiff::FileMap filemap(key_.get_Encryption_Key());
				filemap.set_strong_hash_type(get_strong_hash_type());
				filemap.create(abspath.generic_string());
				meta.set_filemap(filemap);
			}
		}

		if(meta.meta_type() == Meta::SYMLINK){
			// Symlink path = encrypted symlink destination.
			meta.set_symlink_path(fs::read_symlink(abspath).generic_string(), key_);
		}else{
			// File modification time
			meta.set_mtime(fs::last_write_time(abspath));	// TODO: make analogue function for symlinks. Use boost::filesystem::last_write_time as a template. lstat for Unix and GetFileAttributesEx for Windows.
		}

		// Write new values of attributes (if enabled in config)
#if BOOST_OS_WINDOWS
		if(dir_.dir_options().get("preserve_windows_attrib", false)){
			meta.set_windows_attrib(GetFileAttributes(abspath.native().c_str()));	// Windows attributes (I don't have Windows now to test it), this code is stub for now.
		}
#elif BOOST_OS_UNIX
		if(dir_.dir_options().get("preserve_unix_attrib", false)){
			struct stat stat_buf; int stat_err = 0;
			if(dir_.dir_options().get("preserve_symlinks", true))
				stat_err = lstat(abspath.c_str(), &stat_buf);
			else
				stat_err = stat(abspath.c_str(), &stat_buf);
			if(stat_err == 0){
				meta.set_mode(stat_buf.st_mode);
				meta.set_uid(stat_buf.st_uid);
				meta.set_gid(stat_buf.st_gid);
			}
		}
#endif
	}

	// Revision
	meta.set_revision(std::time(nullptr));	// Meta is ready. Assigning timestamp.

	return meta;
}

Meta::SignedMeta Indexer::sign(const Meta& meta) const {
	CryptoPP::AutoSeededRandomPool rng;
	Meta::SignedMeta result;
	result.meta_ = meta.serialize();

	CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Signer signer;
	signer.AccessKey().Initialize(CryptoPP::ASN1::secp256r1(), CryptoPP::Integer(key_.get_Private_Key().data(), key_.get_Private_Key().size()));

	result.signature_.resize(signer.SignatureLength());
	signer.SignMessage(rng, result.meta_.data(), result.meta_.size(), result.signature_.data());
	return result;
}

cryptodiff::StrongHashType Indexer::get_strong_hash_type() {
	return cryptodiff::StrongHashType(dir_.dir_options().get<uint8_t>("block_strong_hash_type", 0));
}

} /* namespace librevault */
