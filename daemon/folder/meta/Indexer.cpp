/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "Indexer.h"

#include "Index.h"
#include "control/FolderParams.h"
#include "control/StateCollector.h"
#include "folder/AbstractFolder.h"
#include "folder/IgnoreList.h"
#include "folder/PathNormalizer.h"
#include "util/byte_convert.h"
#include "util/file_util.h"
#include "util/log.h"
#include <librevault/crypto/HMAC-SHA3.h>
#include <librevault/crypto/AES_CBC.h>
#include <rabin.h>

namespace librevault {

Indexer::Indexer(const FolderParams& params, Index& index, IgnoreList& ignore_list, PathNormalizer& path_normalizer, StateCollector& state_collector, io_service& ios) :
	params_(params),
	index_(index),
	ignore_list_(ignore_list),
	path_normalizer_(path_normalizer),
	state_collector_(state_collector),
	ios_(ios), secret_(params.secret), indexing_now_(0) {
	state_collector_.folder_state_set(secret_.get_Hash(), "is_indexing", false);
}

Indexer::~Indexer() {
	LOGFUNC();
	active = false;
	for(auto& queue_element : index_queue_) {
		queue_element.second.wait();
	}
	LOGFUNCEND();
}

void Indexer::index(const std::string& file_path) noexcept {
	LOGFUNC() << file_path;

	state_collector_.folder_state_set(secret_.get_Hash(), "is_indexing", bool(++indexing_now_));

	SignedMeta smeta;

	try {
		if(ignore_list_.is_ignored(file_path)) throw abort_index("File is ignored");

		try {
			smeta = index_.get_meta(Meta::make_path_id(file_path, secret_));
			if(fs::last_write_time(path_normalizer_.absolute_path(file_path)) == smeta.meta().mtime()) {
				throw abort_index("Modification time is not changed");
			}
		}catch(fs::filesystem_error& e){
		}catch(AbstractFolder::no_such_meta& e){
		}catch(Meta::error& e){
			LOGW("Meta in DB is inconsistent, trying to reindex: " << e.what());
		}

		std::chrono::high_resolution_clock::time_point before_index = std::chrono::high_resolution_clock::now();  // Starting timer
		smeta = make_Meta(file_path);   // Actual indexing
		std::chrono::high_resolution_clock::time_point after_index = std::chrono::high_resolution_clock::now();   // Stopping timer
		float time_spent = std::chrono::duration<float, std::chrono::seconds::period>(after_index - before_index).count();

		index_.put_meta(smeta, true);

		LOGD("Updated index entry in " << time_spent << "s (" << size_to_string((double)smeta.meta().size()/time_spent) << "/s)"
			<< " Path=" << file_path
			<< " Rev=" << smeta.meta().revision()
			<< " Chk=" << smeta.meta().chunks().size());
	}catch(abort_index& e){
		LOGN("Skipping " << file_path << ". Reason: " << e.what());
	}catch(std::runtime_error& e){
		LOGE("Skipping " << file_path << ". Error: " << e.what());
	}

	state_collector_.folder_state_set(secret_.get_Hash(), "is_indexing", bool(--indexing_now_));
}

void Indexer::async_index(const std::string& file_path) {
	if(!active) return;

	std::unique_lock<std::mutex> lk(index_queue_mtx_);
	if(index_queue_.find(file_path) == index_queue_.end()) {
		std::shared_ptr<std::packaged_task<void()>> index_task = std::make_shared<std::packaged_task<void()>>([this, file_path] {
			index(file_path);

			index_queue_mtx_.lock();
			index_queue_.erase(file_path);
			index_queue_mtx_.unlock();
		});

		index_queue_.emplace(std::make_pair(file_path, std::move(index_task->get_future())));

		ios_.post([index_task]{(*index_task)();});
	}
}

void Indexer::async_index(const std::set<std::string>& file_path) {
	LOGD("Preparing to index " << file_path.size() << " entries.");
	for(auto file_path1 : file_path)
		async_index(file_path1);
}

/* Actual indexing process */
SignedMeta Indexer::make_Meta(const std::string& relpath) {
	LOGD("make_Meta(" << relpath << ")");
	Meta old_meta, new_meta;
	auto abspath = path_normalizer_.absolute_path(relpath);

	new_meta.set_path(relpath, secret_);    // sets path_id, encrypted_path and encrypted_path_iv

	new_meta.set_meta_type(get_type(abspath));  // Type

	try {	// Tries to get old Meta from index. May throw if no such meta or if Meta is invalid (parsing failed).
		auto old_smeta = index_.get_meta(new_meta.path_id());
		old_meta = old_smeta.meta();
	}catch(AbstractFolder::no_such_meta& e) {
		if(new_meta.meta_type() == Meta::DELETED)
			throw abort_index("Old Meta is not in the index, new Meta is DELETED");
	}

	if(old_meta.meta_type() == Meta::DIRECTORY && new_meta.meta_type() == Meta::DIRECTORY)
		throw abort_index("Old Meta is DIRECTORY, new Meta is DIRECTORY");

	if(old_meta.meta_type() == Meta::DELETED && new_meta.meta_type() == Meta::DELETED)
		throw abort_index("Old Meta is DELETED, new Meta is DELETED");

	if(new_meta.meta_type() == Meta::FILE)
		update_chunks(old_meta, new_meta, abspath);

	if(new_meta.meta_type() == Meta::SYMLINK)
		new_meta.set_symlink_path(boost::filesystem::read_symlink(abspath).generic_string(), secret_); // Symlink path = encrypted symlink destination.

	// FSAttrib
	if(new_meta.meta_type() != Meta::DELETED)
		update_fsattrib(old_meta, new_meta, abspath);   // Platform-dependent attributes (windows attrib, uid, gid, mode)

	// Revision
	new_meta.set_revision(std::time(nullptr));	// Meta is ready. Assigning timestamp.

	return SignedMeta(new_meta, secret_);
}

Meta::Type Indexer::get_type(const boost::filesystem::path& path) {
	boost::filesystem::file_status file_status = params_.preserve_symlinks ? boost::filesystem::symlink_status(path) : boost::filesystem::status(path);	// Preserves symlinks if such option is set.

	switch(file_status.type()){
		case boost::filesystem::regular_file: return Meta::FILE;
		case boost::filesystem::directory_file: return Meta::DIRECTORY;
		case boost::filesystem::symlink_file: return Meta::SYMLINK;
		case boost::filesystem::file_not_found: return Meta::DELETED;
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
		new_meta.set_mtime(boost::filesystem::last_write_time(path));   // File/directory modification time
	else {
		// TODO: make alternative function for symlinks. Use boost::filesystem::last_write_time as an example. lstat for Unix and GetFileAttributesEx for Windows.
	}

	// Then, write new values of attributes (if enabled in config)
#if BOOST_OS_WINDOWS
	if(params_.preserve_windows_attrib) {
		new_meta.set_windows_attrib(GetFileAttributes(path.native().c_str()));	// Windows attributes (I don't have Windows now to test it), this code is stub for now.
	}
#elif BOOST_OS_UNIX
	if(params_.preserve_unix_attrib) {
		struct stat stat_buf; int stat_err = 0;
		if(params_.preserve_symlinks)
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
		new_meta.set_strong_hash_type(params_.chunk_strong_hash_type);

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

	file_wrapper f(path, "rb");

	while(!f.ios().eof() && active) {
		auto byte_read = f.ios().get(); if(byte_read == EOF) continue;

		buffer.push_back(byte_read);
		//size_t len = fread(buf, 1, sizeof(buf), stdin);
		uint8_t *ptr = &buffer.back();

		if(rabin_next_chunk(&hasher, ptr, 1) == 1) {    // Found a chunk
			chunks.push_back(populate_chunk(new_meta, buffer, pt_hmac__iv));
			buffer.clear();
		}
	}

	if(!active)
		throw abort_index("Application is shutting down");

	if(rabin_finalize(&hasher) != 0)
		chunks.push_back(populate_chunk(new_meta, buffer, pt_hmac__iv));

	new_meta.set_chunks(chunks);
}

Meta::Chunk Indexer::populate_chunk(const Meta& new_meta, const blob& data, const std::map<blob, blob>& pt_hmac__iv) {
	LOGD("New chunk size: " << data.size());
	Meta::Chunk chunk;
	chunk.pt_hmac = data | crypto::HMAC_SHA3_224(secret_.get_Encryption_Key());

	// IV reuse
	auto it = pt_hmac__iv.find(chunk.pt_hmac);
	chunk.iv = (it != pt_hmac__iv.end() ? it->second : crypto::AES_CBC::random_iv());

	chunk.size = data.size();
	chunk.ct_hash = Meta::Chunk::compute_strong_hash(Meta::Chunk::encrypt(data, secret_.get_Encryption_Key(), chunk.iv), new_meta.strong_hash_type());
	return chunk;
}

} /* namespace librevault */
