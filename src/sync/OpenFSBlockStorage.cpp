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
#include "FileMeta.pb.h"
#include "OpenFSBlockStorage.h"
#include "../util.h"
#include <cryptopp/osrng.h>
#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>
#include <fstream>
#include <set>

namespace librevault {

OpenFSBlockStorage::OpenFSBlockStorage(const boost::filesystem::path& dirpath,
		const boost::filesystem::path& dbpath,
		const cryptodiff::key_t& key) : EncFSBlockStorage(dirpath, dbpath), encryption_key(key) {
}

OpenFSBlockStorage::~OpenFSBlockStorage() {}

boost::optional<boost::filesystem::path> OpenFSBlockStorage::relpath(boost::filesystem::path path) const {
	boost::filesystem::path relpath;
	path = boost::filesystem::absolute(path);

	auto path_elem_it = path.begin();
	for(auto dir_elem : directory_path){
		if(dir_elem != *(path_elem_it++))
			return boost::none;
	}
	for(; path_elem_it != path.end(); path_elem_it++){
		if(*path_elem_it == "." || *path_elem_it == "..")
			return boost::none;
		relpath /= *path_elem_it;
	}
	return relpath;
}

void OpenFSBlockStorage::create_index_file(const boost::filesystem::path& filepath){
	FileMeta file_meta;

	// Type
	auto file_status = boost::filesystem::symlink_status(filepath);
	switch(file_status.type()){
		case boost::filesystem::regular_file:
			file_meta.set_type(FileMeta::FileType::FileMeta_FileType_FILE);
			break;
		case boost::filesystem::directory_file:
			file_meta.set_type(FileMeta::FileType::FileMeta_FileType_DIRECTORY);
			break;
		case boost::filesystem::symlink_file:
			file_meta.set_type(FileMeta::FileType::FileMeta_FileType_SYMLINK);
			file_meta.set_symlink_to(boost::filesystem::read_symlink(filepath).generic_string());
			break;
	}

	// IV
	cryptodiff::iv_t iv;
	CryptoPP::AutoSeededRandomPool rng;
	rng.GenerateBlock(iv.data(), iv.size());
	file_meta.set_iv(iv.data(), iv.size());

	// EncPath
	auto relfilepath = relpath(filepath); if(relfilepath == boost::none) throw;
	std::string portable_path = relfilepath.get().generic_string();
	auto encpath = encrypt(reinterpret_cast<const uint8_t*>(portable_path.data()), portable_path.size(), encryption_key, iv);
	file_meta.set_encpath(encpath.data(), encpath.size());

	// mtime
	file_meta.set_mtime(boost::filesystem::last_write_time(filepath));

	// FileMap
	if(file_meta.type() == FileMeta::FileType::FileMeta_FileType_FILE){
		boost::filesystem::ifstream fs(filepath, std::ios_base::binary);
		cryptodiff::FileMap filemap(encryption_key);
		filemap.create(fs);

		file_meta.mutable_filemap()->ParseFromString(filemap.to_string());	// TODO: Amazingly dirty.
	}

	// Windows attributes (I don't have Windows now to test it)
#ifdef _WIN32
	file_meta.set_windows_attrib(GetFileAttributes(filepath.native().c_str()));
#endif

	auto fileid = put_FileMeta(file_meta, sign(file_meta));

	std::string filetype_string;
	switch(file_meta.type()){
		case FileMeta::FileType::FileMeta_FileType_FILE:
			filetype_string = "FILE";
			break;
		case FileMeta::FileType::FileMeta_FileType_DIRECTORY:
			filetype_string = "DIRECTORY";
			break;
		case FileMeta::FileType::FileMeta_FileType_SYMLINK:
			filetype_string = "SYMLINK";
			break;
	}

	BOOST_LOG_TRIVIAL(debug) << "Created new index entry. Path=" << filepath << " Type=" << filetype_string << " ID=" << fileid;
}

void OpenFSBlockStorage::update_index_file(const boost::filesystem::path& filepath){
	if(boost::filesystem::is_regular_file(filepath) && !boost::filesystem::is_symlink(filepath)){
		BOOST_LOG_TRIVIAL(debug) << filepath;

		boost::filesystem::ifstream fs(filepath, std::ios_base::binary);

		std::vector<uint8_t> signature;
		auto old_filemap = get_FileMeta(filepath, signature);
		//auto new_filemap = old_filemap.update(fs);

		//put_EncFileMap(filepath, new_filemap, "durrr", true);	// TODO: signature, of course
	}else{
		// TODO: Do something useful
	}
}

std::vector<uint8_t> OpenFSBlockStorage::sign(const FileMeta& file_meta){
	return std::vector<uint8_t>({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15});
}

void OpenFSBlockStorage::create_index(){
	directory_db.exec("SAVEPOINT create_index;");
	directory_db.exec("DELETE FROM files;");
	directory_db.exec("DELETE FROM blocks;");
	directory_db.exec("DELETE FROM block_locations;");

	for(auto dir_entry_it = boost::filesystem::recursive_directory_iterator(directory_path); dir_entry_it != boost::filesystem::recursive_directory_iterator(); dir_entry_it++){
		create_index_file(dir_entry_it->path());
	}
	directory_db.exec("RELEASE create_index;");
}

FileMeta OpenFSBlockStorage::get_FileMeta(const boost::filesystem::path& filepath, std::vector<uint8_t>& signature){
	auto query_result = directory_db.exec("SELECT meta, signature FROM files WHERE path=:path;", {
			{":path", SQLValue(filepath.generic_string())},
	});

	FileMeta meta;
	if(query_result.have_rows()){
		meta.ParseFromString((*query_result.begin())[0].as_text());
		signature = ((*query_result.begin())[1].as_blob());
	}

	return meta;
}

int64_t OpenFSBlockStorage::put_FileMeta(const FileMeta& meta, const std::vector<uint8_t>& signature) {
	std::vector<uint8_t> old_signature;
	auto old_filemeta = get_FileMeta(std::vector<uint8_t>(meta.encpath().data(), meta.encpath().data()+meta.encpath().size()), old_signature);

	auto old_filemeta_s = old_filemeta.SerializeAsString();
	auto new_filemeta_s = meta.SerializeAsString();
	if(old_filemeta_s != FileMeta().SerializeAsString() && old_filemeta_s != new_filemeta_s){
		cryptodiff::EncFileMap filemap_old;
		filemap_old.from_string(old_filemeta_s);
		cryptodiff::EncFileMap filemap_new;
		filemap_new.from_string(new_filemeta_s);

		auto blocks_to_remove = filemap_old.delta(filemap_new);	// Yes, blocks to remove are new.delta(old). Blocks to obtain are old.delta(new)
		for(auto block : blocks_to_remove){
			auto result = directory_db.exec("SELECT count(id) FROM blocks WHERE encrypted_hash=:encrypted_hash;", {
					{"encrypted_hash", SQLValue(block.get_encrypted_hash().data(), block.get_encrypted_hash().size())}
			});
			int64_t block_count = (*result.begin()).at(0).as_int();
			result.finalize();
			if(block_count <= 1){
				EncFSBlockStorage::remove_block_data(block.get_encrypted_hash());
			}
		}

	}

	auto fileid = EncFSBlockStorage::put_FileMeta(meta, signature, true);
	//
	cryptodiff::iv_t iv; std::copy(meta.iv().begin(), meta.iv().end(), &*iv.begin());
	auto decrypted_path = decrypt(reinterpret_cast<const uint8_t*>(meta.encpath().data()), meta.encpath().size(), encryption_key, iv);

	directory_db.exec("UPDATE files SET path=:path WHERE id=:id;", {
			{":path", decrypted_path},
			{":id", fileid}
	});

	return fileid;
}

std::vector<uint8_t> OpenFSBlockStorage::get_block_data(const cryptodiff::shash_t& block_hash) {
	auto block_data = EncFSBlockStorage::get_block_data(block_hash);
	if(block_data.size() != 0){
		return block_data;
	}else{
		auto result = directory_db.exec("SELECT blocks.blocksize, blocks.iv, files.path, block_locations.offset FROM block_locations "
				"LEFT OUTER JOIN files ON blocks.fileid = files.id "
				"LEFT OUTER JOIN blocks ON blocks.blockid = blocks.id "
				"WHERE encrypted_hash=:encrypted_hash", {
						{":encrypted_hash", SQLValue(block_hash.data(), block_hash.size())}
		});

		for(auto row : result){
			// Blocksize
			auto blocksize = row.at(0).as_int();

			// IV
			cryptodiff::iv_t iv = row.at(1).as_blob<AES_BLOCKSIZE>();

			// Path
			auto filepath = boost::filesystem::absolute(boost::filesystem::path(row.at(2).as_text()), directory_path);

			// Offset
			uint64_t offset = row.at(3).as_int();

			boost::filesystem::ifstream fs(filepath, std::ios_base::binary);
			fs.seekg(offset);
			block_data.resize(blocksize);

			fs.read(reinterpret_cast<char*>(block_data.data()), blocksize);
			if(fs){
				block_data = encrypt(block_data.data(), block_data.size(), encryption_key, iv);
				// Check
				if(compute_shash(block_data.data(), block_data.size()) == block_hash){
					return block_data;
				}
			}
		}

		return std::vector<uint8_t>();
	}
}

void OpenFSBlockStorage::put_block_data(const cryptodiff::shash_t& block_hash, const std::vector<uint8_t>& data) {
	EncFSBlockStorage::put_block_data(block_hash, data);

	auto blocks_to_write = directory_db.exec("SELECT files.path, blocks.encrypted_hash, block_locations.[offset], blocks.iv "
			"FROM ("
				"SELECT DISTINCT blocks.fileid AS unique_fileid "
				"FROM blocks "
				"WHERE blocks.encrypted_hash = :encrypted_hash"
			")"
			"JOIN files ON unique_fileid = files.id "
			"JOIN blocks ON unique_fileid = blocks.fileid "
			"JOIN block_locations ON block_locations.blockid = blocks.id "
			"ORDER BY path;", {
					{":encrypted_hash", SQLValue(block_hash.data(), block_hash.size())}
			});

	std::map<std::string, boost::optional<std::map<uint64_t, std::pair<cryptodiff::shash_t, cryptodiff::iv_t>>>> files_to_renew;

	for(auto block : blocks_to_write){
		auto path = block[0].as_text();
		auto encrypted_hash = block[1].as_blob<SHASH_LENGTH>();
		auto offset = block[2].as_int();
		auto iv = block[3].as_blob<AES_BLOCKSIZE>();

		auto renew_it = files_to_renew.find(path);
		if(renew_it == files_to_renew.end()){
			files_to_renew.insert(std::make_pair(path, std::map<uint64_t, std::pair<cryptodiff::shash_t, cryptodiff::iv_t>>()));
		}else if(renew_it->second == boost::none){
			continue;
		}

		if(EncFSBlockStorage::block_exists(encrypted_hash)){
			renew_it->second->insert(std::make_pair((uint64_t)offset, std::make_pair(encrypted_hash, iv)));
		}else{
			renew_it->second = boost::none;
		}
	}

	for(auto file_to_renew : files_to_renew){
		if(file_to_renew.second != boost::none){
			auto filepath = boost::filesystem::absolute(boost::filesystem::path(file_to_renew.first), directory_path);

			boost::filesystem::ofstream fs(filepath, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
			for(auto block : file_to_renew.second.get()){
				auto block_encrypted = EncFSBlockStorage::get_block_data(block.second.first);
				auto block_decrypted = decrypt(block_encrypted.data(), block_encrypted.size(), encryption_key, block.second.second);
				fs.write((const char*)block_decrypted.data(), block_decrypted.size());
			}
		}
	}
}

} /* namespace librevault */
