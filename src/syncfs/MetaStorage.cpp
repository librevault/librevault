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
#include "MetaStorage.h"
#include <boost/log/trivial.hpp>

namespace librevault {
namespace syncfs {

MetaStorage::MetaStorage(FSBlockStorage* parent) : parent(parent) {}

MetaStorage::~MetaStorage() {}

void MetaStorage::set_key(const crypto::Key& aes_key){
	std::map<int64_t, std::string> paths_to_update;
	auto query_result = parent->directory_db->exec("SELECT id, encpath, encpath_iv FROM files WHERE path=NULL;");
	for(auto row : query_result){
		auto encpath = row[1].as_blob();
		auto path = crypto::decrypt(encpath.data(), encpath.size(), aes_key, row[2].as_blob<crypto::AES_BLOCKSIZE>());
		paths_to_update.insert(std::make_pair(row[0].as_int(), std::string(path.begin(), path.end())));
	}

	for(auto path_to_update : paths_to_update){
		parent->directory_db->exec("UPDATE files SET path=:path WHERE id=:id;", {
				{":path", path_to_update.second},
				{":id", path_to_update.first}
		});
	}

	have_path = true;
}

void MetaStorage::reset_key(){
	parent->directory_db->exec("UPDATE files SET path=NULL;");

	have_path = false;
}

// Encrypted path operations
bool MetaStorage::have_Meta(const blob& encpath) {
	auto query_result = parent->directory_db->exec("SELECT 1 FROM files WHERE encpath=:encpath LIMIT 1;", {
			{":encpath", encpath},
	});

	return query_result.have_rows();
}

MetaStorage::SignedMeta MetaStorage::get_Meta(const blob& encpath) {
	auto query_result = parent->directory_db->exec("SELECT meta, signature FROM files WHERE encpath=:encpath LIMIT 1;", {
			{":encpath", encpath},
	});

	for(auto row : query_result){
		SignedMeta meta;
		meta.meta.ParseFromString(row[0]);
		meta.signature = row[1];

		return meta;
	}
	throw parent->NoSuchMeta;
}

void MetaStorage::remove_Meta(const blob& encpath){
	parent->directory_db->exec("DELETE FROM files WHERE encpath=:encpath LIMIT 1;", {
			{":encpath", encpath},
	});

	BOOST_LOG_TRIVIAL(debug) << "Removed Meta of " << crypto::to_base32(encpath.data(), encpath.size());
}

// Non-encrypted path operations
bool MetaStorage::have_Meta(const fs::path& relpath) {
	auto query_result = parent->directory_db->exec("SELECT 1 FROM files WHERE path=:path LIMIT 1;", {
			{":path", relpath.generic_string()},
	});

	return query_result.have_rows();
}

MetaStorage::SignedMeta MetaStorage::get_Meta(const fs::path& relpath) {
	auto query_result = parent->directory_db->exec("SELECT meta, signature FROM files WHERE path=:path LIMIT 1;", {
			{":path", relpath.generic_string()},
	});

	for(auto row : query_result){
		SignedMeta meta;
		meta.meta.ParseFromString(row[0]);
		meta.signature = row[1];

		return meta;
	}
	throw parent->NoSuchMeta;
}

void MetaStorage::remove_Meta(const fs::path& relpath){
	parent->directory_db->exec("DELETE FROM files WHERE path=:path LIMIT 1;", {
			{":id", relpath.generic_string()},
	});

	BOOST_LOG_TRIVIAL(debug) << "Removed Meta of " << relpath.generic_string();
}

void MetaStorage::put_Meta(const SignedMeta& meta) {
	std::vector<uint8_t> meta_blob(meta.meta.ByteSize());
	meta.meta.SerializeToArray(meta_blob.data(), meta_blob.size());

	SQLValue path;
	if(have_path){
		crypto::IV encpath_iv; std::copy(meta.meta.encpath_iv().begin(), meta.meta.encpath_iv().end(), encpath_iv.data());
		auto path = crypto::decrypt((const uint8_t*)meta.meta.encpath().data(), meta.meta.encpath().size(), parent->get_aes_key(), encpath_iv);
		path = SQLValue(std::string(path.begin(), path.end()));
	}

	parent->directory_db->exec("INSERT OR REPLACE INTO files (path, encpath, encpath_iv, mtime, meta, signature) VALUES (:path, :encpath, :encpath_iv, :mtime, :meta, :signature);", {
			{":path", path},
			{":encpath", SQLValue((const uint8_t*)meta.meta.encpath().data(), meta.meta.encpath().size())},
			{":encpath_iv", SQLValue((const uint8_t*)meta.meta.encpath_iv().data(), meta.meta.encpath_iv().size())},
			{":mtime", meta.meta.mtime()},
			{":meta", meta_blob},
			{":signature", meta.signature}
	});

	BOOST_LOG_TRIVIAL(debug) << "Added Meta of " << have_path ? path.as_text() : crypto::to_base32((const uint8_t*)meta.meta.encpath().data(), meta.meta.encpath().size());
}

std::list<MetaStorage::SignedMeta> MetaStorage::get_custom(const std::string& sql, std::map<std::string, SQLValue> values){
	auto query_result = parent->directory_db->exec(sql, values);

	std::list<MetaStorage::SignedMeta> result_list;
	for(auto row : query_result){
		SignedMeta meta;
		meta.meta.ParseFromString(row[0]);
		meta.signature = row[1];

		result_list.push_back(meta);
	}

	return result_list;
}

std::list<MetaStorage::SignedMeta> MetaStorage::get_all(){
	return get_custom("SELECT meta, signature FROM files;");
}

std::list<MetaStorage::SignedMeta> MetaStorage::get_newer_than(int64_t mtime){
	return get_custom("SELECT meta, signature FROM files WHERE mtime >= :mtime;", {
			{":mtime", mtime}
	});
}

} /* namespace syncfs */
} /* namespace librevault */
