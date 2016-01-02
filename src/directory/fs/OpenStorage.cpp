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
#include "OpenStorage.h"

#include "FSDirectory.h"
#include "IgnoreList.h"
#include "AutoIndexer.h"

namespace librevault {

OpenStorage::OpenStorage(FSDirectory& dir) :
		AbstractStorage(dir), Loggable(dir, "OpenStorage"),
		key_(dir_.key()), index_(*dir_.index), enc_storage_(*dir_.enc_storage) {
	bool open_path_created = fs::create_directories(dir_.open_path());
	log_->debug() << log_tag() << "Open directory: " << dir_.open_path() << (open_path_created ? " created" : "");
	bool asm_path_created = fs::create_directories(dir_.asm_path());
	log_->debug() << log_tag() << "Assemble directory: " << dir_.asm_path() << (asm_path_created ? " created" : "");
}

bool OpenStorage::have_block(const blob& encrypted_data_hash) const {
	auto sql_result = index_.db().exec("SELECT assembled FROM openfs "
										   "WHERE encrypted_data_hash=:encrypted_data_hash", {
										   {":encrypted_data_hash", encrypted_data_hash}
									   });
	for(auto row : sql_result) {
		if(row[0].as_int() > 0) return true;
	}
	return false;
}

std::pair<std::shared_ptr<blob>, std::shared_ptr<blob>> OpenStorage::get_both_blocks(const blob& encrypted_data_hash) const {
	log_->trace() << log_tag() << "get_both_blocks(" << AbstractDirectory::encrypted_data_hash_readable(encrypted_data_hash) << ")";

	auto metas_containing = index_.containing_block(encrypted_data_hash);

	for(auto smeta : metas_containing) {
		// Search for block offset and index
		uint64_t offset = 0;
		unsigned block_idx = 0;
		for(auto& block : smeta.meta().blocks()) {
			if(block.encrypted_data_hash_ == encrypted_data_hash) break;
			offset += block.blocksize_;
			block_idx++;
		}
		if(block_idx > smeta.meta().blocks().size()) continue;

		// Found block & offset
		auto block = smeta.meta().blocks().at(block_idx);
		std::shared_ptr<blob> block_content = std::make_shared<blob>(block.blocksize_);

		fs::ifstream ifs; ifs.exceptions(std::ios::failbit | std::ios::badbit);
		try {
			ifs.open(fs::absolute(smeta.meta().path(key_), dir_.open_path()), std::ios::binary);
			ifs.seekg(offset);
			ifs.read(reinterpret_cast<char*>(block_content->data()), block.blocksize_);

			std::shared_ptr<blob> encblock = std::make_shared<blob>(cryptodiff::encrypt_block(*block_content, key_.get_Encryption_Key(), block.iv_));
			// Check
			if(verify_block(encrypted_data_hash, *encblock, smeta.meta().strong_hash_type())) return {block_content, encblock};
		}catch(const std::ios::failure& e){}
	}
	throw AbstractDirectory::no_such_block();
}

std::shared_ptr<blob> OpenStorage::get_block(const blob& encrypted_data_hash) const {
	log_->trace() << log_tag() << "get_block(" << AbstractDirectory::encrypted_data_hash_readable(encrypted_data_hash) << ")";
	return get_both_blocks(encrypted_data_hash).second;
}

blob OpenStorage::get_openblock(const blob& encrypted_data_hash) const {
	log_->trace() << log_tag() << "get_openblock(" << AbstractDirectory::encrypted_data_hash_readable(encrypted_data_hash) << ")";
	blob block = dir_.get_block(encrypted_data_hash);

	for(auto row : index_.db().exec("SELECT blocksize, iv FROM blocks WHERE encrypted_data_hash=:encrypted_data_hash", {{":encrypted_data_hash", encrypted_data_hash}})) {
		return cryptodiff::decrypt_block(block, row[0].as_uint(), key_.get_Encryption_Key(), row[1].as_blob());
	}
	throw AbstractDirectory::no_such_block();
}

void OpenStorage::assemble(const Meta& meta, bool delete_blocks){
	log_->trace() << log_tag() << "assemble()";

	if(meta.meta_type() == Meta::DELETED) {
		assemble_deleted(meta);
	}else{
		switch(meta.meta_type()) {
			case Meta::SYMLINK:     assemble_symlink(meta); break;
			case Meta::DIRECTORY:   assemble_directory(meta); break;
			case Meta::FILE:        assemble_file(meta, delete_blocks); break;
			default:                throw assemble_error();
		}
		apply_attrib(meta);
	}
}

void OpenStorage::assemble_deleted(const Meta& meta) {
	log_->trace() << log_tag() << "assemble_deleted()";

	fs::path file_path = fs::absolute(meta.path(key_), dir_.open_path());
	auto file_type = fs::symlink_status(file_path).type();

	// Suppress unnecessary events on dir_monitor.
	if(dir_.auto_indexer) dir_.auto_indexer->prepare_deleted_assemble(dir_.make_relpath(file_path));

	if(file_type == fs::symlink_file || file_type == fs::directory_file || file_type == fs::file_not_found)
		fs::remove(file_path);
	else if(fs::is_empty(file_path))
		fs::remove_all(file_path);
	else
		fs::remove(file_path);
}

void OpenStorage::assemble_symlink(const Meta& meta) {
	log_->trace() << log_tag() << "assemble_symlink()";

	fs::path file_path = fs::absolute(meta.path(key_), dir_.open_path());
	fs::remove_all(file_path);
	fs::create_symlink(meta.symlink_path(key_), file_path);
}

void OpenStorage::assemble_directory(const Meta& meta) {
	log_->trace() << log_tag() << "assemble_directory()";

	fs::path file_path = fs::absolute(meta.path(key_), dir_.open_path());
	auto relpath = dir_.make_relpath(file_path);

	bool removed = false;
	if(fs::status(file_path).type() != fs::file_type::directory_file){
		removed = fs::remove(file_path);
	}
	if(dir_.auto_indexer) dir_.auto_indexer->prepare_dir_assemble(removed, relpath);

	fs::create_directories(file_path);
}

void OpenStorage::assemble_file(const Meta& meta, bool delete_blocks) {
	log_->trace() << log_tag() << "assemble_file()";

	fs::path file_path = fs::absolute(meta.path(key_), dir_.open_path());
	auto relpath = dir_.make_relpath(file_path);
	auto assembled_file = dir_.asm_path() / fs::unique_path("assemble-%%%%-%%%%-%%%%-%%%%");

	// TODO: Check for assembled blocks and try to extract them and push into encstorage.
	fs::ofstream ofs(assembled_file, std::ios::out | std::ios::trunc | std::ios::binary);

	for(auto block : meta.blocks()) {
		blob block_data = get_openblock(block.encrypted_data_hash_);
		ofs.write((const char*)block_data.data(), block_data.size());
	}

	fs::last_write_time(assembled_file, meta.mtime());

	//dir_.ignore_list->add_ignored(relpath);
	if(dir_.auto_indexer) dir_.auto_indexer->prepare_file_assemble(fs::exists(file_path), relpath);

	fs::remove(file_path);
	fs::rename(assembled_file, file_path);
	//dir_.ignore_list->remove_ignored(relpath);

	index_.db().exec("UPDATE openfs SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta.path_id()}});

	if(delete_blocks){
		for(auto block : meta.blocks()) {
			enc_storage_.remove_block(block.encrypted_data_hash_);
		}
	}
}

void OpenStorage::apply_attrib(const Meta& meta) {
	fs::path file_path = fs::absolute(meta.path(key_), dir_.open_path());

#if BOOST_OS_UNIX
	if(dir_.dir_options().get("preserve_unix_attrib", false)) {
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
void OpenStorage::disassemble(const std::string& file_path, bool delete_file){
	blob path_id = make_path_id(file_path);

	std::list<blob> block_encrypted_hashes;
	auto blocks_data = index_.db().exec("SELECT block_encrypted_hash "
			"FROM openfs WHERE path_id=:path_id", {
					{":path_id", path_id}
			});
	for(auto row : blocks_data)
		block_encrypted_hashes.push_back(row[0]);
	for(auto block_encrypted_hash : block_encrypted_hashes)
		enc_storage_.put_encblock(block_encrypted_hash, get_encblock(block_encrypted_hash));

	if(delete_file){
		index_.db().exec("UPDATE openfs SET assembled=0 WHERE file_path_hmac=:path_hmac", {
				{":path_hmac", path_id}
		});
		fs::remove(fs::absolute(file_path, dir_.open_path()));
	}
}
*/
blob OpenStorage::make_path_id(const std::string& relpath) const {
	return relpath | crypto::HMAC_SHA3_224(key_.get_Encryption_Key());
}

std::set<std::string> OpenStorage::open_files(){
	std::set<std::string> file_list;

	for(auto dir_entry_it = fs::recursive_directory_iterator(dir_.open_path()); dir_entry_it != fs::recursive_directory_iterator(); dir_entry_it++){
		auto relpath = dir_.make_relpath(dir_entry_it->path());

		if(!dir_.ignore_list->is_ignored(relpath)) file_list.insert(relpath);
	}
	return file_list;
}

std::set<std::string> OpenStorage::indexed_files() {
	std::set<std::string> file_list;

	for(auto smeta : index_.get_Meta()) {
		std::string relpath = smeta.meta().path(key_);
		if(!dir_.ignore_list->is_ignored(relpath)) file_list.insert(relpath);
	}

	return file_list;
}

std::set<std::string> OpenStorage::pending_files(){
	std::set<std::string> file_list = open_files();

	for(auto smeta : index_.get_Meta()) {
		if(smeta.meta().meta_type() != Meta::DELETED){
			file_list.insert(smeta.meta().path(key_));
		}
	}

	return file_list;
}

std::set<std::string> OpenStorage::all_files(){
	std::set<std::string> file_list1, file_list2;
	file_list1 = open_files();
	file_list2 = indexed_files();

	file_list1.insert(file_list2.begin(), file_list2.end());
	return file_list1;
}

} /* namespace librevault */
