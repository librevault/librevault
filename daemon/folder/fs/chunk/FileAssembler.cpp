/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "FileAssembler.h"

#include "Client.h"

#include "folder/fs/FSFolder.h"
#include "folder/fs/IgnoreList.h"
#include "folder/fs/AutoIndexer.h"

#include "ChunkStorage.h"

namespace librevault {

FileAssembler::FileAssembler(FSFolder& dir, ChunkStorage& chunk_storage, Client& client) :
	Loggable(dir, "FileAssembler"),
	dir_(dir),
	chunk_storage_(chunk_storage),
	client_(client),
	archive_(dir_, client_),
	secret_(dir_.secret()),
	index_(*dir_.index),
	periodic_assemble_timer_(client_.bulk_ios()) {

	periodic_assemble_operation();
}

blob FileAssembler::get_chunk_pt(const blob& ct_hash) const {
	log_->trace() << log_tag() << "get_chunk_pt(" << AbstractFolder::ct_hash_readable(ct_hash) << ")";
	blob chunk = chunk_storage_.get_chunk(ct_hash);

	for(auto row : index_.db().exec("SELECT size, iv FROM chunk WHERE ct_hash=:ct_hash", {{":ct_hash", ct_hash}})) {
		return Meta::Chunk::decrypt(chunk, row[0].as_uint(), secret_.get_Encryption_Key(), row[1].as_blob());
	}
	throw AbstractFolder::no_such_chunk();
}

void FileAssembler::queue_assemble(const Meta& meta) {
	assemble_queue_mtx_.lock();
	if(assemble_queue_.find(meta.path_id()) == assemble_queue_.end()) {
		assemble_queue_.insert(meta.path_id());

		client_.bulk_ios().post([this, meta]() {
			assemble(meta);

			assemble_queue_mtx_.lock();
			assemble_queue_.erase(meta.path_id());
			assemble_queue_mtx_.unlock();
		});
	}

	assemble_queue_mtx_.unlock();
}

void FileAssembler::periodic_assemble_operation() {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	log_->debug() << log_tag() << "Performing periodic assemble";

	for(auto smeta : dir_.index->get_incomplete_meta()) {
		queue_assemble(smeta.meta());
	}

	periodic_assemble_timer_.expires_from_now(std::chrono::seconds(30));
	periodic_assemble_timer_.async_wait([this](boost::system::error_code ec){
		if(ec == boost::asio::error::operation_aborted) {
			log_->debug() << log_tag() << "periodic_assemble_operation returned";
			return;
		}
		periodic_assemble_operation();
	});
}

void FileAssembler::assemble(const Meta& meta){
	log_->trace() << log_tag() << "assemble()";

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

			index_.db().exec("UPDATE meta SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta.path_id()}});
		}
	}catch(std::runtime_error& e) {
		log_->warn() << log_tag() << BOOST_CURRENT_FUNCTION << " path:" << meta.path(secret_) << " e:" << e.what(); // FIXME: Plaintext path in logs may violate user's privacy.
	}
}

bool FileAssembler::assemble_deleted(const Meta& meta) {
	log_->trace() << log_tag() << "assemble_deleted()";

	fs::path file_path = dir_.absolute_path(meta.path(secret_));
	auto file_type = fs::symlink_status(file_path).type();

	// Suppress unnecessary events on dir_monitor.
	if(dir_.auto_indexer) dir_.auto_indexer->prepare_deleted_assemble(dir_.normalize_path(file_path));

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
	log_->trace() << log_tag() << "assemble_symlink()";

	fs::path file_path = dir_.absolute_path(meta.path(secret_));
	fs::remove_all(file_path);
	fs::create_symlink(meta.symlink_path(secret_), file_path);

	return true;    // Maybe, something else?
}

bool FileAssembler::assemble_directory(const Meta& meta) {
	log_->trace() << log_tag() << "assemble_directory()";

	fs::path file_path = dir_.absolute_path(meta.path(secret_));
	auto relpath = dir_.normalize_path(file_path);

	bool create_new = true;
	if(fs::status(file_path).type() != fs::file_type::directory_file)
		create_new = !fs::remove(file_path);
	if(dir_.auto_indexer) dir_.auto_indexer->prepare_dir_assemble(create_new, relpath);

	if(create_new) fs::create_directories(file_path);

	return true;    // Maybe, something else?
}

bool FileAssembler::assemble_file(const Meta& meta) {
	log_->trace() << log_tag() << "assemble_file()";

	// Check if we have all needed chunks
	if(!chunk_storage_.make_bitfield(meta).all())
		return false; // retreat!

	//
	fs::path file_path = dir_.absolute_path(meta.path(secret_));
	auto relpath = dir_.normalize_path(file_path);
	auto assembled_file = dir_.system_path() / fs::unique_path("assemble-%%%%-%%%%-%%%%-%%%%");

	// TODO: Check for assembled chunk and try to extract them and push into encstorage.
	fs::ofstream ofs(assembled_file, std::ios::out | std::ios::trunc | std::ios::binary);	// Opening file

	for(auto chunk : meta.chunks()) {
		blob chunk_pt = get_chunk_pt(chunk.ct_hash);
		ofs.write((const char*)chunk_pt.data(), chunk_pt.size());	// Writing to file
	}

	ofs.close();	// Closing file. Super!

	fs::last_write_time(assembled_file, meta.mtime());

	//dir_.ignore_list->add_ignored(relpath);
	if(dir_.auto_indexer) dir_.auto_indexer->prepare_file_assemble(fs::exists(file_path), relpath);

	archive_.archive(file_path);
	fs::rename(assembled_file, file_path);
	//dir_.ignore_list->remove_ignored(relpath);

	index_.db().exec("UPDATE openfs SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta.path_id()}});

	chunk_storage_.cleanup(meta);

	return true;
}

void FileAssembler::apply_attrib(const Meta& meta) {
	fs::path file_path = dir_.absolute_path(meta.path(secret_));

#if BOOST_OS_UNIX
	if(dir_.params().preserve_unix_attrib) {
		if(meta.meta_type() != Meta::SYMLINK) {
			int ec = 0;
			ec = chmod(file_path.c_str(), meta.mode());
			if(ec) log_->warn() << log_tag() << "Error applying mode to " << file_path;  // FIXME: full_path in logs may violate user's privacy.
			ec = chown(file_path.c_str(), meta.uid(), meta.gid());
			if(ec) log_->warn() << log_tag() << "Error applying uid/gid to " << file_path;  // FIXME: full_path in logs may violate user's privacy.
		}
	}
#elif BOOST_OS_WINDOWS
	// Apply Windows attrib here.
#endif
}

} /* namespace librevault */
