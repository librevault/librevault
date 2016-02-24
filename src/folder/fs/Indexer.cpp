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
#include "Indexer.h"

#include "FSFolder.h"
#include "IgnoreList.h"

#include "../../Client.h"
#include "../../util/byte_convert.h"

namespace librevault {

Indexer::Indexer(FSFolder& dir, Client& client) :
	Loggable(dir, "Indexer"), dir_(dir),
	secret_(dir_.secret()), index_(*dir_.index), enc_storage_(*dir_.enc_storage), open_storage_(*dir_.open_storage), client_(client) {}

void Indexer::index(const std::string& file_path){
	log_->trace() << log_tag() << "Indexer::index(" << file_path << ")";

	Meta::SignedMeta smeta;

	try {
		if(dir_.ignore_list->is_ignored(file_path)) throw error("File is ignored");

		try {
			smeta = index_.get_Meta(open_storage_.make_path_id(file_path));
			if(fs::last_write_time(fs::absolute(file_path, dir_.open_path())) == smeta.meta().mtime()) {
				throw error("Modification time is not changed");
			}
		}catch(fs::filesystem_error& e){
		}catch(AbstractFolder::no_such_meta& e){
		}catch(Meta::SignedMeta::signature_error& e){   // Alarm! DB is inconsistent
			log_->warn() << log_tag() << "Signature mismatch in local DB";
		}

		std::chrono::steady_clock::time_point before_index = std::chrono::steady_clock::now();  // Starting timer
		smeta = make_Meta(file_path);   // Actual indexing
		std::chrono::steady_clock::time_point after_index = std::chrono::steady_clock::now();   // Stopping timer
		float time_spent = std::chrono::duration<float, std::chrono::seconds::period>(after_index - before_index).count();

		log_->trace() << log_tag() << smeta.meta().debug_string();
		log_->debug() << log_tag() << "Updated index entry in " << time_spent << "s (" << size_to_string((double)smeta.meta().size()/time_spent) << "/s)" << ". Path=" << file_path << " Rev=" << smeta.meta().revision();
	}catch(std::runtime_error& e){
		log_->notice() << log_tag() << "Skipping " << file_path << ". Reason: " << e.what();
	}

	if(smeta) dir_.put_meta(smeta, true);
}

void Indexer::async_index(const std::string& file_path) {
	client_.ios().post(std::bind([this](const std::string& file_path){
		index(file_path);
	}, file_path));
}

void Indexer::async_index(const std::set<std::string>& file_path) {
	log_->debug() << log_tag() << "Preparing to index " << file_path.size() << " entries.";
	for(auto file_path1 : file_path)
		async_index(file_path1);
}

Meta::SignedMeta Indexer::make_Meta(const std::string& relpath){
	Meta meta;
	auto abspath = fs::absolute(relpath, dir_.open_path());

	// Path ID aka HMAC of relpath aka SHA3(key | relpath);
	meta.set_path_id(open_storage_.make_path_id(relpath));
	// Encrypted path with IV
	meta.set_path(relpath, secret_);

	// Type
	meta.set_meta_type(get_type(abspath));

	if(meta.meta_type() != Meta::DELETED){
		try {	// Tries to get old Meta from index. May throw if no such meta or if Meta is invalid (parsing failed).
			Meta old_meta(index_.get_Meta(meta.path_id()).meta());

			// Preserve old values of attributes
			meta.set_windows_attrib(old_meta.windows_attrib());
			meta.set_mode(old_meta.mode());
			meta.set_uid(old_meta.uid());
			meta.set_gid(old_meta.gid());

			if(meta.meta_type() == Meta::FILE){
				// Update FileMap
				cryptodiff::FileMap filemap = old_meta.filemap(secret_);
				filemap.update(abspath.generic_string());
				meta.set_filemap(filemap);
			}
		}catch(...){
			if(meta.meta_type() == Meta::FILE){
				// Create FileMap, if something got wrong
				cryptodiff::FileMap filemap(secret_.get_Encryption_Key());
				filemap.set_strong_hash_type(get_strong_hash_type());
				filemap.update(abspath.generic_string());
				meta.set_filemap(filemap);
			}
		}

		if(meta.meta_type() == Meta::SYMLINK){
			// Symlink path = encrypted symlink destination.
			meta.set_symlink_path(fs::read_symlink(abspath).generic_string(), secret_);
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
		if(dir_.folder_config().preserve_unix_attrib){
			struct stat stat_buf; int stat_err = 0;
			if(dir_.folder_config().preserve_symlinks)
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

	return Meta::SignedMeta(meta, secret_);
}

Meta::Type Indexer::get_type(const fs::path& path) {
	fs::file_status file_status = dir_.folder_config().preserve_symlinks ? fs::symlink_status(path) : fs::status(path);	// Preserves symlinks if such option is set.

	switch(file_status.type()){
		case fs::regular_file: return Meta::FILE;
		case fs::directory_file: return Meta::DIRECTORY;
		case fs::symlink_file: return Meta::SYMLINK;
		case fs::file_not_found: return Meta::DELETED;
		default: throw unsupported_filetype();
	}
}

cryptodiff::StrongHashType Indexer::get_strong_hash_type() {
	return dir_.folder_config().block_strong_hash_type;
}

} /* namespace librevault */
