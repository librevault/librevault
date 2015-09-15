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
#include "Meta.pb.h"

namespace librevault {

OpenStorage::OpenStorage(FSDirectory& dir, Session& session) :
		log_(spdlog::get("Librevault")), dir_(dir),
		key_(dir_.key()), index_(*dir_.index), enc_storage_(*dir_.enc_storage) {
	bool open_path_created = fs::create_directories(dir_.open_path());
	log_->debug() << "Open directory: " << dir_.open_path() << (open_path_created ? " created" : "");
	bool asm_path_created = fs::create_directories(dir_.asm_path().parent_path());
	log_->debug() << "Assemble directory: " << dir_.asm_path().parent_path() << (asm_path_created ? " created" : "");
}
OpenStorage::~OpenStorage() {}

std::string OpenStorage::get_path(const blob& path_hmac){
	for(auto row : index_.db().exec("SELECT meta FROM files WHERE path_hmac=:path_hmac", {{":path_hmac", path_hmac}})){
		std::vector<uint8_t> meta_bytes = row[0];
		Meta meta; meta.ParseFromArray(meta_bytes.data(), meta_bytes.size());
		return (std::string)crypto::AES_CBC(key_.get_Encryption_Key(), meta.encpath_iv()).from(meta.encpath());
	}
	throw;
}

std::pair<blob, blob> OpenStorage::get_both_blocks(const blob& block_hash){
	auto sql_result = index_.db().exec("SELECT blocks.blocksize, blocks.iv, files.meta, openfs.[offset] FROM blocks "
			"JOIN openfs ON blocks.encrypted_hash = openfs.block_encrypted_hash "
			"JOIN files ON openfs.file_path_hmac = files.path_hmac "
			"WHERE blocks.encrypted_hash=:encrypted_hash", {
					{":encrypted_hash", block_hash}
			});

	for(auto row : sql_result){
		Meta meta; meta.ParseFromString(row[2].as_text());

		uint64_t blocksize	= row[0];
		blob iv				= row[1];
		auto filepath		= fs::absolute((std::string)crypto::AES_CBC(key_.get_Encryption_Key(), meta.encpath_iv()).from(meta.encpath()), dir_.open_path());
		uint64_t offset		= row[3];

		fs::ifstream ifs; ifs.exceptions(std::ios::failbit | std::ios::badbit);
		blob block(blocksize);

		try {
			ifs.open(filepath, std::ios::binary);
			ifs.seekg(offset);
			ifs.read(reinterpret_cast<char*>(block.data()), blocksize);

			blob encblock = crypto::AES_CBC(key_.get_Encryption_Key(), iv, blocksize % 16 == 0 ? false : true).encrypt(block);
			// Check
			if(verify_encblock(block_hash, encblock)) return {block, encblock};
		}catch(const std::ios::failure& e){}
	}
	throw no_such_block();
}

blob OpenStorage::get_encblock(const blob& block_hash){
	try {
		return enc_storage_.get_encblock(block_hash);
	}catch(no_such_block& e){
		return get_both_blocks(block_hash).second;
	}
}

blob OpenStorage::get_block(const blob& block_hash) {
	try {
		for(auto row : index_.db().exec("SELECT iv FROM blocks WHERE encrypted_hash=:encrypted_hash", {{":encrypted_hash", block_hash}})){
			blob encblock = enc_storage_.get_encblock(block_hash);
			return crypto::AES_CBC(key_.get_Encryption_Key(), row[0].as_blob(), encblock.size() % 16 == 0 ? false : true).from(encblock);
		}
	}catch(no_such_block& e){
		return get_both_blocks(block_hash).first;
	}
	return blob();
}

void OpenStorage::assemble(bool delete_blocks){
	auto raii_lock = SQLiteLock(index_.db());
	auto blocks = index_.db().exec("SELECT files.path_hmac, openfs.\"offset\", blocks.encrypted_hash, blocks.iv "
			"FROM openfs "
			"JOIN files ON openfs.file_path_hmac=files.path_hmac "
			"JOIN blocks ON openfs.block_encrypted_hash=blocks.encrypted_hash "
			"WHERE assembled=0");
	std::map<blob, std::map<uint64_t, std::pair<blob, blob>>> write_blocks;
	for(auto row : blocks){
		blob path_hmac = row[0].as_blob();
		uint64_t offset = row[1].as_uint();
		blob enchash = row[2].as_blob();
		blob enchash_iv = row[3].as_blob();
		write_blocks[path_hmac][offset] = {enchash, enchash_iv};
	}

	for(auto file : write_blocks){
		fs::ofstream ofs(dir_.asm_path(), std::ios::out | std::ios::trunc | std::ios::binary);

		blob path_hmac = file.first;
		for(auto block : file.second){
			//auto offset = block.first;	// Unused
			auto block_hash = block.second.first;
			auto iv = block.second.second;

			blob block_data = get_block(block_hash);
			ofs.write((const char*)block_data.data(), block_data.size());
		}
		ofs.close();

		auto abspath = fs::absolute(get_path(path_hmac), dir_.open_path());
		fs::remove(abspath);
		fs::rename(dir_.asm_path(), abspath);

		index_.db().exec("UPDATE openfs SET assembled=1 WHERE file_path_hmac=:file_path_hmac", {{":file_path_hmac", path_hmac}});
	}
}

void OpenStorage::disassemble(const std::string& file_path, bool delete_file){
	blob path_hmac = make_path_hmac(file_path);

	std::list<blob> encrypted_hashes;
	auto blocks_data = index_.db().exec("SELECT block_encrypted_hash "
			"FROM openfs WHERE file_path_hmac=:file_path_hmac", {
					{":file_path_hmac", path_hmac}
			});
	for(auto row : blocks_data)
		encrypted_hashes.push_back(row[0]);
	for(auto encrypted_hash : encrypted_hashes)
		enc_storage_.put_encblock(encrypted_hash, get_encblock(encrypted_hash));

	if(delete_file){
		index_.db().exec("UPDATE openfs SET assembled=0 WHERE file_path_hmac=:path_hmac", {
				{":path_hmac", path_hmac}
		});
		fs::remove(fs::absolute(file_path, dir_.open_path()));
	}
}

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

blob OpenStorage::make_path_hmac(const std::string& relpath) const {
	return crypto::HMAC_SHA3_224(key_.get_Encryption_Key()).to(relpath);
}

bool OpenStorage::is_skipped(const std::string relpath) const {
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
		Meta meta; meta.ParseFromString(row[0].as_text());
		std::string relpath = crypto::AES_CBC(key_.get_Encryption_Key(), meta.encpath_iv()).from(meta.encpath());

		if(!is_skipped(relpath)) file_list.insert(relpath);
	}

	return file_list;
}

std::set<std::string> OpenStorage::pending_files(){
	std::set<std::string> file_list1, file_list2;
	file_list1 = open_files();

	for(auto row : index_.db().exec("SELECT meta FROM files")){
		Meta meta; meta.ParseFromString(row[0].as_text());
		if(meta.type() != meta.DELETED){
			file_list1.insert(crypto::AES_CBC(key_.get_Encryption_Key(), meta.encpath_iv()).from(meta.encpath()));
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
