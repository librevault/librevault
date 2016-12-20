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
#include "FileAssembler.h"

#include "ChunkStorage.h"
#include "control/FolderParams.h"
#include "folder/AbstractFolder.h"
#include "folder/IgnoreList.h"
#include "folder/PathNormalizer.h"
#include "folder/meta/Index.h"
#include "folder/meta/MetaStorage.h"
#include "util/file_util.h"
#include "util/log.h"

namespace librevault {

FileAssembler::FileAssembler(const FolderParams& params,
                             MetaStorage& meta_storage,
                             ChunkStorage& chunk_storage,
                             PathNormalizer& path_normalizer,
                             Archive& archive,
                             io_service& bulk_ios,
                             io_service& serial_ios) :
	params_(params),
	meta_storage_(meta_storage),
	chunk_storage_(chunk_storage),
	path_normalizer_(path_normalizer),
	archive_(archive),
	bulk_ios_(bulk_ios),
	secret_(params_.secret),
	assemble_timer_(serial_ios),
	current_assemble_(0) {

	assemble_timer_.tick_signal.connect([this]{periodic_assemble_operation();});
	assemble_timer_.start(std::chrono::seconds(30), ScopedTimer::RUN_IMMEDIATELY, ScopedTimer::RESET_TIMER, ScopedTimer::NOT_SINGLESHOT);
}

FileAssembler::~FileAssembler() {
	std::unique_lock<std::mutex> lk(assemble_queue_mtx_);
	assemble_queue_.clear();

	while(current_assemble_ != 0)
		std::this_thread::yield();
}

blob FileAssembler::get_chunk_pt(const blob& ct_hash) const {
	LOGT("get_chunk_pt(" << AbstractFolder::ct_hash_readable(ct_hash) << ")");
	blob chunk = chunk_storage_.get_chunk(ct_hash);

	for(auto row : meta_storage_.index->db().exec("SELECT size, iv FROM chunk WHERE ct_hash=:ct_hash", {{":ct_hash", ct_hash}})) {
		return Meta::Chunk::decrypt(chunk, row[0].as_uint(), secret_.get_Encryption_Key(), row[1].as_blob());
	}
	throw AbstractFolder::no_such_chunk();
}

void FileAssembler::queue_assemble(const Meta& meta) {
	std::unique_lock<std::mutex> lk(assemble_queue_mtx_);
	if(assemble_queue_.find(meta.path_id()) == assemble_queue_.end()) {
		assemble_queue_.insert(meta.path_id());

		bulk_ios_.post([this, meta]() {
			++current_assemble_;
			assemble(meta);
			--current_assemble_;

			std::unique_lock<std::mutex> lk(assemble_queue_mtx_);
			assemble_queue_.erase(meta.path_id());
		});
	}
}

void FileAssembler::periodic_assemble_operation() {
	LOGFUNC();
	LOGT("Performing periodic assemble");

	for(auto smeta : meta_storage_.index->get_incomplete_meta())
		queue_assemble(smeta.meta());
}

void FileAssembler::assemble(const Meta& meta){
	LOGFUNC();

	try {
		bool assembled = false;
		switch(meta.meta_type()) {
			case Meta::FILE: assembled = assemble_file(meta);
				break;
			case Meta::DIRECTORY: assembled = assemble_directory(meta);
				break;
			case Meta::SYMLINK: assembled = assemble_symlink(meta);
				break;
			case Meta::DELETED: assembled = assemble_deleted(meta);
				break;
			default: throw error(std::string("Unexpected meta type:") + std::to_string(meta.meta_type()));
		}
		if(assembled) {
			if(meta.meta_type() != Meta::DELETED)
				apply_attrib(meta);

			meta_storage_.index->db().exec("UPDATE meta SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta.path_id()}});
		}
	}catch(std::runtime_error& e) {
		LOGW(BOOST_CURRENT_FUNCTION << " path:" << meta.path(secret_) << " e:" << e.what()); // FIXME: Plaintext path in logs may violate user's privacy.
	}
}

bool FileAssembler::assemble_deleted(const Meta& meta) {
	LOGFUNC();

	fs::path file_path = path_normalizer_.absolute_path(meta.path(secret_));
	auto file_type = fs::symlink_status(file_path).type();

	// Suppress unnecessary events on dir_monitor.
	meta_storage_.prepare_assemble(path_normalizer_.normalize_path(file_path), Meta::DELETED);

	if(file_type == fs::directory_file) {
		if(fs::is_empty(file_path)) // Okay, just remove this empty directory
			fs::remove(file_path);
		else  // Oh, damn, this is very NOT RIGHT! So, we have DELETED directory with NOT DELETED files in it
			fs::remove_all(file_path);  // TODO: Okay, this is a horrible solution
	}

	if(file_type == fs::symlink_file || file_type == fs::file_not_found)
		fs::remove(file_path);
	else if(file_type == fs::regular_file)
		archive_.archive(file_path);
	// TODO: else

	return true;    // Maybe, something else?
}

bool FileAssembler::assemble_symlink(const Meta& meta) {
	LOGFUNC();

	fs::path file_path = path_normalizer_.absolute_path(meta.path(secret_));
	fs::remove_all(file_path);
	fs::create_symlink(meta.symlink_path(secret_), file_path);

	return true;    // Maybe, something else?
}

bool FileAssembler::assemble_directory(const Meta& meta) {
	LOGFUNC();

	fs::path file_path = path_normalizer_.absolute_path(meta.path(secret_));
	auto relpath = path_normalizer_.normalize_path(file_path);

	bool create_new = true;
	if(fs::status(file_path).type() != fs::file_type::directory_file)
		create_new = !fs::remove(file_path);
	meta_storage_.prepare_assemble(relpath, Meta::DIRECTORY, create_new);

	if(create_new) fs::create_directories(file_path);

	return true;    // Maybe, something else?
}

bool FileAssembler::assemble_file(const Meta& meta) {
	LOGFUNC();

	// Check if we have all needed chunks
	if(!chunk_storage_.make_bitfield(meta).all())
		return false; // retreat!

	//
	fs::path file_path = path_normalizer_.absolute_path(meta.path(secret_));
	auto relpath = path_normalizer_.normalize_path(file_path);
	auto assembled_file = params_.system_path / fs::unique_path("assemble-%%%%-%%%%-%%%%-%%%%");

	// TODO: Check for assembled chunk and try to extract them and push into encstorage.
	file_wrapper assembling_file(assembled_file, "wb"); // Opening file

	for(auto chunk : meta.chunks()) {
		blob chunk_pt = get_chunk_pt(chunk.ct_hash);
		assembling_file.ios().write((const char*)chunk_pt.data(), chunk_pt.size());	// Writing to file
	}

	assembling_file.close();	// Closing file. Super!

	fs::last_write_time(assembled_file, meta.mtime());

	//dir_.ignore_list->add_ignored(relpath);
	meta_storage_.prepare_assemble(relpath, Meta::FILE, fs::exists(file_path));

	archive_.archive(file_path);
	fs::rename(assembled_file, file_path);
	//dir_.ignore_list->remove_ignored(relpath);

	meta_storage_.index->db().exec("UPDATE openfs SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta.path_id()}});

	chunk_storage_.cleanup(meta);

	return true;
}

void FileAssembler::apply_attrib(const Meta& meta) {
	fs::path file_path = path_normalizer_.absolute_path(meta.path(secret_));

#if BOOST_OS_UNIX
	if(params_.preserve_unix_attrib) {
		if(meta.meta_type() != Meta::SYMLINK) {
			int ec = 0;
			ec = chmod(file_path.c_str(), meta.mode());
			if(ec) LOGW("Error applying mode to " << file_path);    // FIXME: full_path in logs may violate user's privacy.
			ec = chown(file_path.c_str(), meta.uid(), meta.gid());
			if(ec) LOGW("Error applying uid/gid to " << file_path); // FIXME: full_path in logs may violate user's privacy.
		}
	}
#elif BOOST_OS_WINDOWS
	// Apply Windows attrib here.
#endif
}

} /* namespace librevault */
