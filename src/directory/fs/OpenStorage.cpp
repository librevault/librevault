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

#include "../Meta.h"

namespace librevault {

OpenStorage::OpenStorage(FSDirectory& dir) :
		AbstractStorage(dir),
		key_(dir_.key()), index_(*dir_.index), enc_storage_(*dir_.enc_storage) {
	bool open_path_created = fs::create_directories(dir_.open_path());
	log_->debug() << log_tag() << "Open directory: " << dir_.open_path() << (open_path_created ? " created" : "");
	bool asm_path_created = fs::create_directories(dir_.asm_path().parent_path());
	log_->debug() << log_tag() << "Assemble directory: " << dir_.asm_path().parent_path() << (asm_path_created ? " created" : "");
}

bool OpenStorage::have_block(const blob& encrypted_data_hash) {
	auto sql_result = index_.db().exec("SELECT assembled FROM openfs "
										   "WHERE encrypted_data_hash=:encrypted_data_hash", {
										   {":encrypted_data_hash", encrypted_data_hash}
									   });
	for(auto row : sql_result) {
		if(row[0].as_int() > 0) return true;
	}
	return false;
}

std::pair<std::shared_ptr<blob>, std::shared_ptr<blob>> OpenStorage::get_both_blocks(const blob& encrypted_data_hash){
	auto sql_result = index_.db().exec("SELECT blocks.blocksize, blocks.iv, files.meta, openfs.[offset] FROM blocks "
			"JOIN openfs ON blocks.encrypted_data_hash = openfs.encrypted_data_hash "
			"JOIN files ON openfs.path_id = files.path_id "
			"WHERE blocks.encrypted_data_hash=:encrypted_data_hash", {
					{":encrypted_data_hash", encrypted_data_hash}
			});

	for(auto row : sql_result){
		Meta meta(row[2].as_blob());

		uint64_t blocksize	= row[0];
		blob iv 			= row[1];
		auto filepath		= fs::absolute(meta.path(key_), dir_.open_path());
		uint64_t offset		= row[3];

		fs::ifstream ifs; ifs.exceptions(std::ios::failbit | std::ios::badbit);
		std::shared_ptr<blob> block = std::make_shared<blob>(blocksize);

		try {
			ifs.open(filepath, std::ios::binary);
			ifs.seekg(offset);
			ifs.read(reinterpret_cast<char*>(block->data()), blocksize);

			std::shared_ptr<blob> encblock = std::make_shared<blob>(cryptodiff::encrypt_block(*block, key_.get_Encryption_Key(), iv));
			// Check
			if(verify_block(encrypted_data_hash, *encblock, meta.strong_hash_type())) return {block, encblock};
		}catch(const std::ios::failure& e){}
	}
	throw AbstractDirectory::no_such_block();
}

std::shared_ptr<blob> OpenStorage::get_block(const blob& encrypted_data_hash){
	return get_both_blocks(encrypted_data_hash).second;
}

blob OpenStorage::get_openblock(const blob& encrypted_data_hash) {
	try {
		for(auto row : index_.db().exec("SELECT iv, blocksize FROM blocks WHERE encrypted_data_hash=:encrypted_data_hash", {{":encrypted_data_hash", encrypted_data_hash}})){
			auto encblock_ptr = enc_storage_.get_block(encrypted_data_hash);
			return cryptodiff::decrypt_block(*encblock_ptr, row[1].as_uint(), key_.get_Encryption_Key(), row[0].as_blob());
		}
		throw AbstractDirectory::no_such_block();
	}catch(AbstractDirectory::no_such_block& e){
		return *get_both_blocks(encrypted_data_hash).first;
	}
}

void OpenStorage::assemble(const Meta& meta, bool delete_blocks){
	fs::path file_path = fs::absolute(meta.path(key_), dir_.open_path());
	auto relpath = dir_.make_relpath(file_path);

	if(meta.meta_type() == Meta::DELETED) {
		auto file_type = fs::symlink_status(file_path).type();

		if(dir_.auto_indexer) dir_.auto_indexer->prepare_deleted_assemble(relpath);

		if(file_type == fs::symlink_file || file_type == fs::directory_file || file_type == fs::file_not_found)
			fs::remove(file_path);
		else if(fs::is_empty(file_path))
			fs::remove_all(file_path);
		else
			fs::remove(file_path);
	}else{
		switch(meta.meta_type()) {
			case Meta::SYMLINK: {
				fs::remove_all(file_path);
				fs::create_symlink(meta.symlink_path(key_), file_path);
			} break;
			case Meta::DIRECTORY: {
				bool removed = false;
				if(fs::status(file_path).type() != fs::file_type::directory_file){
					removed = fs::remove(file_path);
				}
				if(dir_.auto_indexer) dir_.auto_indexer->prepare_dir_assemble(removed, relpath);

				fs::create_directories(file_path);
			} break;
			case Meta::FILE: {
				for(auto block : meta.blocks()){
					if(! have_block(block.encrypted_data_hash_)) throw AbstractDirectory::no_such_block();
				}

				SQLiteLock raii_lock(index_.db());
				// Check for assembled blocks and try to extract them and push into encstorage.
				auto blocks = index_.db().exec("SELECT openfs.\"offset\", blocks.encrypted_data_hash, blocks.iv "
												"FROM openfs "
												"JOIN blocks ON openfs.encrypted_data_hash=blocks.encrypted_data_hash "
												"WHERE assembled=0 AND openfs.path_id=:path_id ORDER BY openfs.\"offset\" ASC", {{":path_id", meta.path_id()}});

				fs::ofstream ofs(dir_.asm_path(), std::ios::out | std::ios::trunc | std::ios::binary);

				std::vector<blob> pending_remove;
				for(auto row : blocks){
					//uint64_t offset = row[0].as_uint();
					blob encrypted_data_hash = row[1].as_blob();
					blob iv = row[2].as_blob();

					blob block_data = get_openblock(encrypted_data_hash);

					ofs.write((const char*)block_data.data(), block_data.size());
					pending_remove.push_back(encrypted_data_hash);
				}

				fs::last_write_time(dir_.asm_path(), meta.mtime());

				//dir_.ignore_list->add_ignored(relpath);
				if(dir_.auto_indexer) dir_.auto_indexer->prepare_file_assemble(fs::exists(file_path), relpath);

				fs::remove(file_path);
				fs::rename(dir_.asm_path(), file_path);
				//dir_.ignore_list->remove_ignored(relpath);

				index_.db().exec("UPDATE openfs SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta.path_id()}});

				if(delete_blocks){
					for(auto& encrypted_data_hash : pending_remove) {
						enc_storage_.remove_block(encrypted_data_hash);
					}
				}
			} break;
			default: throw assemble_error();
		}

#if BOOST_OS_UNIX
		if(dir_.dir_options().get("preserve_unix_attrib", false)){
			if(meta.meta_type() != Meta::SYMLINK) {
				(void)chmod(file_path.c_str(), meta.mode());
				(void)chown(file_path.c_str(), meta.uid(), meta.gid());
			}
		}
#elif BOOST_OS_WINDOWS

#endif
	}
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

std::set<std::string> OpenStorage::indexed_files(){
	std::set<std::string> file_list;

	for(auto row : index_.db().exec("SELECT meta FROM files")){
		Meta meta; meta.parse(row[0].as_blob());
		std::string relpath = meta.path(key_);

		if(!dir_.ignore_list->is_ignored(relpath)) file_list.insert(relpath);
	}

	return file_list;
}

std::set<std::string> OpenStorage::pending_files(){
	std::set<std::string> file_list = open_files();

	for(auto row : index_.db().exec("SELECT meta FROM files")){
		Meta meta(row[0].as_blob());
		if(meta.meta_type() != meta.DELETED){
			file_list.insert(meta.path(key_));
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
