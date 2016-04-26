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
	Loggable(dir, "FileAssembler"), dir_(dir), chunk_storage_(chunk_storage), client_(client),
	secret_(dir_.secret()), index_(*dir_.index) {}

blob FileAssembler::get_chunk_pt(const blob& ct_hash) const {
	log_->trace() << log_tag() << "get_chunk_pt(" << AbstractFolder::ct_hash_readable(ct_hash) << ")";
	blob chunk = chunk_storage_.get_chunk(ct_hash);

	for(auto row : index_.db().exec("SELECT size, iv FROM chunk WHERE ct_hash=:ct_hash", {{":ct_hash", ct_hash}})) {
		return Meta::Chunk::decrypt(chunk, row[0].as_uint(), secret_.get_Encryption_Key(), row[1].as_blob());
	}
	throw AbstractFolder::no_such_chunk();
}

void FileAssembler::queue_assemble(const Meta& meta) {
	client_.bulk_ios().dispatch([this, meta](){
		assemble(meta);
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

	fs::path file_path = fs::absolute(meta.path(secret_), dir_.path());
	auto file_type = fs::symlink_status(file_path).type();

	// Suppress unnecessary events on dir_monitor.
	if(dir_.auto_indexer) dir_.auto_indexer->prepare_deleted_assemble(dir_.normalize_path(file_path));

	if(file_type == fs::directory_file) {
		if(fs::is_empty(file_path)) // Okay, just remove this empty directory
			fs::remove(file_path);
		else  // Oh, damn, this is very NOT RIGHT! So, we have DELETED directory with NOT DELETED files in it
			fs::remove_all(file_path);  // TODO: Okay, this is a horrible solution
	}

	if(file_type == fs::regular_file || file_type == fs::symlink_file || file_type == fs::file_not_found)
		fs::remove(file_path);
	// TODO: else

	return true;    // Maybe, something else?
}

bool FileAssembler::assemble_symlink(const Meta& meta) {
	log_->trace() << log_tag() << "assemble_symlink()";

	fs::path file_path = fs::absolute(meta.path(secret_), dir_.path());
	fs::remove_all(file_path);
	fs::create_symlink(meta.symlink_path(secret_), file_path);

	return true;    // Maybe, something else?
}

bool FileAssembler::assemble_directory(const Meta& meta) {
	log_->trace() << log_tag() << "assemble_directory()";

	fs::path file_path = fs::absolute(meta.path(secret_), dir_.path());
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
	fs::path file_path = fs::absolute(meta.path(secret_), dir_.path());
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

	fs::remove(file_path);
	fs::rename(assembled_file, file_path);
	//dir_.ignore_list->remove_ignored(relpath);

	index_.db().exec("UPDATE openfs SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta.path_id()}});

	chunk_storage_.cleanup(meta);

	return true;
}

void FileAssembler::apply_attrib(const Meta& meta) {
	fs::path file_path = fs::absolute(meta.path(secret_), dir_.path());

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

/*
void FileAssembler::disassemble(const std::string& file_path, bool delete_file){
	blob path_id = make_path_id(file_path);

	std::list<blob> chunk_encrypted_hashes;
	auto chunks_data = index_.db().exec("SELECT chunk_encrypted_hash "
			"FROM openfs WHERE path_id=:path_id", {
					{":path_id", path_id}
			});
	for(auto row : chunks_data)
		chunk_encrypted_hashes.push_back(row[0]);
	for(auto chunk_encrypted_hash : chunk_encrypted_hashes)
		enc_storage_.put_encchunk(chunk_encrypted_hash, get_encchunk(chunk_encrypted_hash));

	if(delete_file){
		index_.db().exec("UPDATE openfs SET assembled=0 WHERE file_path_hmac=:path_hmac", {
				{":path_hmac", path_id}
		});
		fs::remove(fs::absolute(file_path, dir_.path()));
	}
}
*/

} /* namespace librevault */
