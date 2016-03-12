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

#include "src/Client.h"
#include "src/util/byte_convert.h"
#include "contrib/rabin/rabin.h"

namespace librevault {

Indexer::Indexer(FSFolder& dir, Client& client) :
	Loggable(dir, "Indexer"), dir_(dir),
	secret_(dir_.secret()), index_(*dir_.index), enc_storage_(*dir_.enc_storage), open_storage_(*dir_.open_storage), client_(client) {}

void Indexer::index(const std::string& file_path){
	log_->trace() << log_tag() << "Indexer::index(" << file_path << ")";

	SignedMeta smeta;

	try {
		if(dir_.ignore_list->is_ignored(file_path)) throw error("File is ignored");

		try {
			smeta = index_.get_Meta(Meta::make_path_id(file_path, secret_));
			if(fs::last_write_time(fs::absolute(file_path, dir_.path())) == smeta.meta().mtime()) {
				throw abort_index("Modification time is not changed");
			}
		}catch(fs::filesystem_error& e){
		}catch(AbstractFolder::no_such_meta& e){
		}catch(Meta::error& e){
			log_->warn() << log_tag() << "Meta in DB is inconsistent, trying to reindex: " << e.what();
		}

		std::chrono::steady_clock::time_point before_index = std::chrono::steady_clock::now();  // Starting timer
		smeta = make_Meta(file_path);   // Actual indexing
		std::chrono::steady_clock::time_point after_index = std::chrono::steady_clock::now();   // Stopping timer
		float time_spent = std::chrono::duration<float, std::chrono::seconds::period>(after_index - before_index).count();

		log_->debug() << log_tag() << "Updated index entry in " << time_spent << "s (" << size_to_string((double)smeta.meta().size()/time_spent) << "/s)"
			<< " Path=" << file_path
			<< " Rev=" << smeta.meta().revision()
			<< " Chk=" << smeta.meta().chunks().size();
	}catch(abort_index& e){
		log_->notice() << log_tag() << "Skipping " << file_path << ". Reason: " << e.what();
	}catch(std::runtime_error& e){
		log_->warn() << log_tag() << "Skipping " << file_path << ". Error: " << e.what();
	}

	if(smeta) dir_.put_meta(smeta, true);
}

void Indexer::async_index(const std::string& file_path) {
	client_.bulk_ios().post(std::bind([this](const std::string& file_path){
		index(file_path);
	}, file_path));
}

void Indexer::async_index(const std::set<std::string>& file_path) {
	log_->debug() << log_tag() << "Preparing to index " << file_path.size() << " entries.";
	for(auto file_path1 : file_path)
		async_index(file_path1);
}

SignedMeta Indexer::make_Meta(const std::string& relpath) {
	log_->debug() << log_tag() << "make_Meta(" << relpath << ")";
	Meta old_meta, new_meta;
	auto abspath = fs::absolute(relpath, dir_.path());

	new_meta.set_path(relpath, secret_);    // sets path_id, encrypted_path and encrypted_path_iv

	new_meta.set_meta_type(get_type(abspath));  // Type
	if(new_meta.meta_type() != Meta::DELETED)
		update_fsattrib(old_meta, new_meta, abspath);   // Platform-dependent attributes (windows attrib, uid, gid, mode)

	try {	// Tries to get old Meta from index. May throw if no such meta or if Meta is invalid (parsing failed).
		auto old_smeta = index_.get_Meta(new_meta.path_id());
		old_meta = old_smeta.meta();
	}catch(...){
		if(new_meta.meta_type() == Meta::DELETED) throw abort_index("Old Meta is not in the index, new Meta is DELETED");
	}

	if(old_meta.meta_type() == Meta::DELETED && new_meta.meta_type() == Meta::DELETED)
		throw abort_index("Old Meta is DELETED, new Meta is DELETED");

	if(new_meta.meta_type() == Meta::FILE)
		update_chunks(old_meta, new_meta, abspath);

	if(new_meta.meta_type() == Meta::SYMLINK)
		new_meta.set_symlink_path(fs::read_symlink(abspath).generic_string(), secret_); // Symlink path = encrypted symlink destination.

	// Revision
	new_meta.set_revision(std::time(nullptr));	// Meta is ready. Assigning timestamp.

	return SignedMeta(new_meta, secret_);
}

Meta::Type Indexer::get_type(const fs::path& path) {
	fs::file_status file_status = dir_.params().preserve_symlinks ? fs::symlink_status(path) : fs::status(path);	// Preserves symlinks if such option is set.

	switch(file_status.type()){
		case fs::regular_file: return Meta::FILE;
		case fs::directory_file: return Meta::DIRECTORY;
		case fs::symlink_file: return Meta::SYMLINK;
		case fs::file_not_found: return Meta::DELETED;
		default: throw unsupported_filetype();
	}
}

void Indexer::update_fsattrib(const Meta& old_meta, Meta& new_meta, const fs::path& path) {
	// First, preserve old values of attributes
	new_meta.set_windows_attrib(old_meta.windows_attrib());
	new_meta.set_mode(old_meta.mode());
	new_meta.set_uid(old_meta.uid());
	new_meta.set_gid(old_meta.gid());

	if(new_meta.meta_type() != Meta::SYMLINK)
		new_meta.set_mtime(fs::last_write_time(path));   // File/directory modification time
	else {
		// TODO: make alternative function for symlinks. Use boost::filesystem::last_write_time as an example. lstat for Unix and GetFileAttributesEx for Windows.
	}

	// Then, write new values of attributes (if enabled in config)
#if BOOST_OS_WINDOWS
	if(dir_.dir_options().get("preserve_windows_attrib", false)) {
		meta.set_windows_attrib(GetFileAttributes(abspath.native().c_str()));	// Windows attributes (I don't have Windows now to test it), this code is stub for now.
	}
#elif BOOST_OS_UNIX
	if(dir_.params().preserve_unix_attrib) {
		struct stat stat_buf; int stat_err = 0;
		if(dir_.params().preserve_symlinks)
			stat_err = lstat(path.c_str(), &stat_buf);
		else
			stat_err = stat(path.c_str(), &stat_buf);
		if(stat_err == 0){
			new_meta.set_mode(stat_buf.st_mode);
			new_meta.set_uid(stat_buf.st_uid);
			new_meta.set_gid(stat_buf.st_gid);
		}
	}
#endif
}

void Indexer::update_chunks(const Meta& old_meta, Meta& new_meta, const fs::path& path) {
	Meta::RabinGlobalParams rabin_global_params;

	if(old_meta.meta_type() == Meta::FILE && old_meta.validate()) {
		new_meta.set_algorithm_type(old_meta.algorithm_type());
		new_meta.set_strong_hash_type(old_meta.strong_hash_type());

		new_meta.set_max_chunksize(old_meta.max_chunksize());
		new_meta.set_min_chunksize(old_meta.min_chunksize());

		new_meta.raw_rabin_global_params() = old_meta.raw_rabin_global_params();

		rabin_global_params = old_meta.rabin_global_params(secret_);
	}else{
		new_meta.set_algorithm_type(Meta::RABIN);
		new_meta.set_strong_hash_type(dir_.params().chunk_strong_hash_type);

		new_meta.set_max_chunksize(8*1024*1024);
		new_meta.set_min_chunksize(1*1024*1024);

		// TODO: Generate a new polynomial for rabin_global_params here to prevent a possible fingerprinting attack.
	}

	// IV reuse
	std::map<blob, blob> pt_hmac__iv;
	for(auto& chunk : old_meta.chunks()) {
		pt_hmac__iv.insert({chunk.pt_hmac, chunk.iv});
	}

	// Initializing chunker
	rabin_t hasher;
	hasher.average_bits = rabin_global_params.avg_bits;
	hasher.minsize = new_meta.min_chunksize();
	hasher.maxsize = new_meta.max_chunksize();
	hasher.polynomial = rabin_global_params.polynomial;
	hasher.polynomial_degree = rabin_global_params.polynomial_degree;
	hasher.polynomial_shift = rabin_global_params.polynomial_shift;

	hasher.mask = uint64_t((1<<uint64_t(hasher.average_bits))-1);

	rabin_init(&hasher);

	// Chunking
	std::vector<Meta::Chunk> chunks;

	blob buffer;
	buffer.reserve(hasher.maxsize);

	fs::ifstream fs(path, std::ios_base::binary);

	while(!fs.eof()) {
		buffer.push_back(fs.get());
		//size_t len = fread(buf, 1, sizeof(buf), stdin);
		uint8_t *ptr = &buffer.back();

		if(rabin_next_chunk(&hasher, ptr, 1) == 1) {    // Found a chunk
			chunks.push_back(populate_chunk(new_meta, buffer));
			buffer.clear();
		}
	}

	if(rabin_finalize(&hasher) != 0)
		chunks.push_back(populate_chunk(new_meta, buffer));

	new_meta.set_chunks(chunks);
}

Meta::Chunk Indexer::populate_chunk(const Meta& new_meta, const blob& data) {
	log_->debug() << log_tag() << data.size();
	Meta::Chunk chunk;
	chunk.pt_hmac = data | crypto::HMAC_SHA3_224(secret_.get_Encryption_Key());
	chunk.iv = crypto::AES_CBC::random_iv();
	chunk.size = data.size();
	chunk.ct_hash = Meta::Chunk::compute_strong_hash(Meta::Chunk::encrypt(data, secret_.get_Encryption_Key(), chunk.iv), new_meta.strong_hash_type());
	return chunk;
}

} /* namespace librevault */
