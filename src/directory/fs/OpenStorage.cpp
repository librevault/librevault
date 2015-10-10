/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "OpenStorage.h"
#include "FSDirectory.h"
#include "../Meta.h"

namespace librevault {

OpenStorage::OpenStorage(FSDirectory& dir, Session& session) :
		log_(spdlog::get("Librevault")), dir_(dir),
		key_(dir_.key()), index_(*dir_.index), enc_storage_(*dir_.enc_storage) {
	bool open_path_created = fs::create_directories(dir_.open_path());
	log_->debug() << dir_.log_tag() << "Open directory: " << dir_.open_path() << (open_path_created ? " created" : "");
	bool asm_path_created = fs::create_directories(dir_.asm_path().parent_path());
	log_->debug() << dir_.log_tag() << "Assemble directory: " << dir_.asm_path().parent_path() << (asm_path_created ? " created" : "");
}
OpenStorage::~OpenStorage() {}

std::string OpenStorage::get_path(const blob& path_id){
	for(auto row : index_.db().exec("SELECT meta FROM files WHERE path_id=:path_id", {{":path_id", path_id}})){
		Meta meta(row[0].as_blob());
		return meta.path(key_);
	}
	throw;
}

std::pair<blob, blob> OpenStorage::get_both_blocks(const blob& encrypted_data_hash){
	auto sql_result = index_.db().exec("SELECT blocks.blocksize, blocks.iv, files.meta, openfs.[offset] FROM blocks "
			"JOIN openfs ON blocks.encrypted_data_hash = openfs.encrypted_data_hash "
			"JOIN files ON openfs.path_id = files.path_id "
			"WHERE blocks.encrypted_data_hash=:encrypted_data_hash", {
					{":encrypted_data_hash", encrypted_data_hash}
			});

	for(auto row : sql_result){
		Meta meta(row[2].as_blob());

		uint64_t blocksize	= row[0];
		blob iv				= row[1];
		auto filepath		= fs::absolute(meta.path(key_), dir_.open_path());
		uint64_t offset		= row[3];

		fs::ifstream ifs; ifs.exceptions(std::ios::failbit | std::ios::badbit);
		blob block(blocksize);

		try {
			ifs.open(filepath, std::ios::binary);
			ifs.seekg(offset);
			ifs.read(reinterpret_cast<char*>(block.data()), blocksize);

			blob encblock = cryptodiff::encrypt_block(block, key_.get_Encryption_Key(), iv);
			// Check
			if(verify_encblock(encrypted_data_hash, encblock, meta.strong_hash_type())) return {block, encblock};
		}catch(const std::ios::failure& e){}
	}
	throw no_such_block();
}

blob OpenStorage::get_encblock(const blob& encrypted_data_hash){
	try {
		return enc_storage_.get_encblock(encrypted_data_hash);
	}catch(no_such_block& e){
		return get_both_blocks(encrypted_data_hash).second;
	}
}

blob OpenStorage::get_block(const blob& encrypted_data_hash) {
	try {
		for(auto row : index_.db().exec("SELECT iv, blocksize FROM blocks WHERE encrypted_data_hash=:encrypted_data_hash", {{":encrypted_data_hash", encrypted_data_hash}})){
			blob encblock = enc_storage_.get_encblock(encrypted_data_hash);
			return cryptodiff::decrypt_block(encblock, row[1].as_uint(), key_.get_Encryption_Key(), row[0].as_blob());
		}
	}catch(no_such_block& e){
		return get_both_blocks(encrypted_data_hash).first;
	}
	return blob();
}

void OpenStorage::assemble(const Meta& meta, bool delete_blocks){
	fs::path file_path = fs::absolute(meta.path(key_), dir_.open_path());

	if(meta.meta_type() == Meta::DELETED) {
		if(fs::is_empty(file_path))
			fs::remove_all(file_path);
		else if(fs::status(file_path).type() != fs::directory_file)
			fs::remove(file_path);
	}else{
		switch(meta.meta_type()) {
			case Meta::SYMLINK: {
				fs::remove_all(file_path);
				fs::create_symlink(meta.symlink_path(key_), file_path);
			} break;
			case Meta::DIRECTORY: {
				fs::remove(file_path);
				fs::create_directories(file_path);
			} break;
			case Meta::FILE: {
				SQLiteLock raii_lock(index_.db());

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

					blob block_data = get_block(encrypted_data_hash);

					ofs.write((const char*)block_data.data(), block_data.size());
					pending_remove.push_back(encrypted_data_hash);
				}

				fs::remove(file_path);
				fs::rename(dir_.asm_path(), file_path);

				index_.db().exec("UPDATE openfs SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta.path_id()}});

				if(delete_blocks){
					for(auto& encrypted_data_hash : pending_remove) {
						enc_storage_.remove_encblock(encrypted_data_hash);
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
				fs::last_write_time(file_path, meta.mtime());
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
std::string OpenStorage::make_relpath(const fs::path& path) const {
	fs::path rel_to = dir_.open_path();
	auto abspath = fs::absolute(path);

	fs::path relpath;
	auto path_elem_it = abspath.begin();
	for(auto dir_elem : rel_to){
		if(dir_elem != *(path_elem_it++))
			return std::string();
	}
	for(; path_elem_it != abspath.end(); path_elem_it++){
		if(*path_elem_it == "." || *path_elem_it == "..")
			return std::string();
		relpath /= *path_elem_it;
	}
	return relpath.generic_string();
}

blob OpenStorage::make_path_id(const std::string& relpath) const {
	return relpath | crypto::HMAC_SHA3_224(key_.get_Encryption_Key());
}

bool OpenStorage::is_skipped(const std::string& relpath) const {
	for(auto& skip_file : skip_files())
		if(relpath.size() >= skip_file.size() && std::equal(skip_file.begin(), skip_file.end(), relpath.begin()))
			return true;
	return false;
}

const std::set<std::string>& OpenStorage::skip_files() const {
	if(!skip_files_.empty()) return skip_files_;

	// Config paths
	auto ignore_list_its = dir_.dir_options().equal_range("ignore");
	for(auto ignore_list_it = ignore_list_its.first; ignore_list_it != ignore_list_its.second; ignore_list_it++){
		skip_files_.insert(ignore_list_it->second.get_value<fs::path>().generic_string());
	}

	// Predefined paths
	skip_files_.insert(make_relpath(dir_.block_path()));
	skip_files_.insert(make_relpath(dir_.db_path()));
	skip_files_.insert(make_relpath(dir_.db_path())+"-journal");
	skip_files_.insert(make_relpath(dir_.db_path())+"-wal");
	skip_files_.insert(make_relpath(dir_.db_path())+"-shm");
	skip_files_.insert(make_relpath(dir_.asm_path()));

	skip_files_.erase(std::string());	// If one (or more) of the above returned empty string (aka if one (or more) of the above paths are outside open_path)

	return skip_files_;
}

std::set<std::string> OpenStorage::open_files(){
	std::set<std::string> file_list;

	for(auto dir_entry_it = fs::recursive_directory_iterator(dir_.open_path()); dir_entry_it != fs::recursive_directory_iterator(); dir_entry_it++){
		auto relpath = make_relpath(dir_entry_it->path());

		if(!is_skipped(relpath)) file_list.insert(relpath);
	}
	return file_list;
}

std::set<std::string> OpenStorage::indexed_files(){
	std::set<std::string> file_list;

	for(auto row : index_.db().exec("SELECT meta FROM files")){
		Meta meta; meta.parse(row[0].as_blob());
		std::string relpath = meta.path(key_);

		if(!is_skipped(relpath)) file_list.insert(relpath);
	}

	return file_list;
}

std::set<std::string> OpenStorage::pending_files(){
	std::set<std::string> file_list1, file_list2;// TODO: WTF?
	file_list1 = open_files();

	for(auto row : index_.db().exec("SELECT meta FROM files")){
		Meta meta; meta.parse(row[0].as_blob());
		if(meta.meta_type() != meta.DELETED){
			file_list1.insert(meta.path(key_));
		}
	}

	file_list1.insert(file_list2.begin(), file_list2.end());
	return file_list1;
}

std::set<std::string> OpenStorage::all_files(){
	std::set<std::string> file_list1, file_list2;
	file_list1 = open_files();
	file_list2 = indexed_files();

	file_list1.insert(file_list2.begin(), file_list2.end());
	return file_list1;
}

} /* namespace librevault */
