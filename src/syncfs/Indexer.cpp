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
#include "Indexer.h"
#include "FSBlockStorage.h"
#include "../../contrib/cryptowrappers/cryptowrappers.h"
#include <cryptopp/osrng.h>
#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>
#include <fstream>

namespace librevault {
namespace syncfs {

Indexer::Indexer(FSBlockStorage* parent) : parent(parent) {}
Indexer::~Indexer() {}

Meta Indexer::make_created_Meta(const fs::path& relpath, bool without_filemap){
	Meta meta;

	auto abspath = fs::absolute(relpath, parent->open_path);

	// Type
	auto file_status = fs::symlink_status(abspath);
	switch(file_status.type()){
		case fs::regular_file:
			meta.set_type(Meta::FileType::Meta_FileType_FILE); break;
		case fs::directory_file:
			meta.set_type(Meta::FileType::Meta_FileType_DIRECTORY); break;
		case fs::symlink_file:
			meta.set_type(Meta::FileType::Meta_FileType_SYMLINK);
			meta.set_symlink_to(fs::read_symlink(abspath).generic_string());
			break;
	}

	// EncPath_IV
	CryptoPP::AutoSeededRandomPool rng;
	crypto::IV encpath_iv;
	rng.GenerateBlock(encpath_iv.data(), encpath_iv.size());
	meta.set_encpath_iv(encpath_iv.data(), encpath_iv.size());

	// EncPath
	std::string portable_path = relpath.generic_string();
	auto encpath = crypto::encrypt(reinterpret_cast<const uint8_t*>(portable_path.data()), portable_path.size(), parent->get_aes_key(), encpath_iv);
	meta.set_encpath(encpath.data(), encpath.size());

	// mtime
	meta.set_mtime(fs::last_write_time(abspath));

	// FileMap
	if(!without_filemap && meta.type() == Meta::FileType::Meta_FileType_FILE){
		fs::ifstream ifs(abspath, std::ios_base::binary);
		cryptodiff::FileMap filemap(parent->get_aes_key());
		filemap.create(ifs);

		meta.mutable_filemap()->ParseFromString(filemap.to_string());	// TODO: Amazingly dirty.
	}

	// Windows attributes (I don't have Windows now to test it)
#ifdef _WIN32
	meta.set_windows_attrib(GetFileAttributes(relpath.native().c_str()));
#endif
	return meta;
}

Meta Indexer::make_updated_Meta(const fs::path& relpath){
	Meta meta = parent->meta_storage->get_Meta(relpath);

	auto new_meta = make_created_Meta(relpath, true);	// Generating new Meta without FileMap.

	// Updating FileMap
	cryptodiff::FileMap new_filemap(parent->get_aes_key());
	new_filemap.from_string(meta.filemap().SerializeAsString());
	auto updated_file = fs::ifstream(fs::absolute(relpath, parent->open_path), std::ios_base::binary);
	new_filemap.update(updated_file);

	meta.MergeFrom(new_meta);
	meta.mutable_filemap()->ParseFromString(new_filemap.to_string());
	return meta;
}

Meta Indexer::make_deleted_Meta(const fs::path& relpath){
	Meta meta;
	meta.set_type(Meta_FileType_DELETED);

	Meta meta_old = parent->meta_storage->get_Meta(relpath);

	meta.set_encpath(meta_old.encpath());
	meta.set_encpath_iv(meta_old.encpath_iv());
	meta.set_mtime((int64_t)std::time(nullptr));

	return meta;
}

void Indexer::create_index(){
	parent->directory_db->exec("SAVEPOINT create_index;");
	parent->directory_db->exec("DELETE FROM files;");
	parent->directory_db->exec("DELETE FROM blocks;");

	for(auto dir_entry_it = fs::recursive_directory_iterator(parent->open_path); dir_entry_it != fs::recursive_directory_iterator(); dir_entry_it++){
		create_index_file(dir_entry_it->path());
	}
	parent->directory_db->exec("RELEASE create_index;");
}

void Indexer::update_index() {
	parent->directory_db->exec("SAVEPOINT update_index;");
	parent->directory_db->exec("CREATE TEMP TABLE IF NOT EXISTS unupdated_files(id INTEGER PRIMARY KEY) WITHOUT ROWID;");
	parent->directory_db->exec("DELETE FROM unupdated_files;");
	parent->directory_db->exec("INSERT INTO unupdated_files (id) SELECT id FROM files;");

	for(auto dir_entry_it = fs::recursive_directory_iterator(parent->open_path); dir_entry_it != fs::recursive_directory_iterator(); dir_entry_it++){
		auto sql_query = parent->directory_db->exec("SELECT id FROM files WHERE path=:path;", {
				{":path", make_relpath(dir_entry_it->path())->generic_string()}
		});
		SQLValue id((int64_t)0);
		if(sql_query.have_rows())
			id = sql_query.begin()->at(0);
		sql_query.finalize();

		if(id){
			update_index_file(path);
			parent->directory_db->exec("DELETE FROM unupdated_files WHERE id=:id;", {
					{":id", id}
			});
		}else{
			create_index_file(dir_entry_it->path());
		}
	}

	auto unupdated_files = parent->directory_db->exec("SELECT path FROM unupdated_files JOIN files ON unupdated_files.id=files.id;");
	for(auto unupdated_file : unupdated_files){
		delete_index_file(unupdated_file[0].as_text());
	}

	parent->directory_db->exec("RELEASE update_index;");
}

void Indexer::create_index_file(const fs::path& relpath){
	auto meta = make_created_Meta(relpath);
	parent->meta_storage->put_Meta(make_signature(meta));

	BOOST_LOG_TRIVIAL(debug) << "Created index entry. Path=" << relpath << " Type=" << make_filetype_string(meta.type());
}

void Indexer::update_index_file(const fs::path& relpath){
	auto meta = make_updated_Meta(relpath);
	parent->meta_storage->put_Meta(make_signature(meta));

	BOOST_LOG_TRIVIAL(debug) << "Updated index entry. Path=" << relpath << " Type=" << make_filetype_string(meta.type());
}

void Indexer::delete_index_file(const fs::path& relpath){
	auto meta = make_deleted_Meta(relpath);
	parent->meta_storage->put_Meta(make_signature(meta));

	BOOST_LOG_TRIVIAL(debug) << "Deleted index entry. Path=" << relpath << " Type=" << make_filetype_string(meta.type());
}

MetaStorage::SignedMeta Indexer::make_signature(const Meta& meta){
	return std::vector<uint8_t>({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15});
}

boost::optional<fs::path> Indexer::make_relpath(const fs::path& path) const {
	fs::path rel_to = parent->open_path;
	path = fs::absolute(path);

	fs::path relpath;
	auto path_elem_it = path.begin();
	for(auto dir_elem : rel_to){
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

} /* namespace syncfs */
} /* namespace librevault */
