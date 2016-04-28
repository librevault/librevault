/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "Index.h"
#include "FSFolder.h"
#include "Client.h"

namespace librevault {

Index::Index(FSFolder& dir, Client& client) : Loggable(dir, "Index"), dir_(dir), client_(client) {
	auto db_filepath = dir_.system_path() / "librevault.db";

	if(fs::exists(db_filepath))
		log_->debug() << log_tag() << "Opening SQLite3 DB: " << db_filepath;
	else
		log_->debug() << log_tag() << "Creating new SQLite3 DB: " << db_filepath;
	db_ = std::make_unique<SQLiteDB>(db_filepath);
	db_->exec("PRAGMA foreign_keys = ON;");

	/* TABLE meta */
	db_->exec("CREATE TABLE IF NOT EXISTS meta (path_id BLOB PRIMARY KEY NOT NULL, meta BLOB NOT NULL, signature BLOB NOT NULL, type INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");
	db_->exec("CREATE INDEX IF NOT EXISTS meta_type_idx ON meta (type);");   // For making "COUNT(*) ... WHERE type=x" way faster
	db_->exec("CREATE INDEX IF NOT EXISTS meta_not_deleted_idx ON meta(type<>255);");   // For faster Index::get_existing_meta

	/* TABLE chunk */
	db_->exec("CREATE TABLE IF NOT EXISTS chunk (ct_hash BLOB NOT NULL PRIMARY KEY, size INTEGER NOT NULL, iv BLOB NOT NULL);");

	/* TABLE openfs */
	db_->exec("CREATE TABLE IF NOT EXISTS openfs (ct_hash BLOB NOT NULL REFERENCES chunk (ct_hash) ON DELETE CASCADE ON UPDATE CASCADE, path_id BLOB NOT NULL REFERENCES meta (path_id) ON DELETE CASCADE ON UPDATE CASCADE, [offset] INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");
	db_->exec("CREATE INDEX IF NOT EXISTS openfs_assembled_idx ON openfs (ct_hash, assembled) WHERE assembled = 1;");    // For faster OpenStorage::have_chunk
	db_->exec("CREATE INDEX IF NOT EXISTS openfs_path_id_fki ON openfs (path_id);");    // For faster FileAssembler::assemble_file
	db_->exec("CREATE IF NOT EXISTS INDEX openfs_ct_hash_fki ON openfs (ct_hash);");    // For faster Index::containing_chunk
	db_->exec("CREATE TRIGGER IF NOT EXISTS chunk_deleter AFTER DELETE ON openfs BEGIN DELETE FROM chunk WHERE ct_hash NOT IN (SELECT ct_hash FROM openfs); END;");

	/* Create a special hash-file */
	auto hash_txt = dir_.system_path() / "hash.txt";
	fs::fstream ifs;
	std::string hexhash_conf = crypto::Hex().to_string(dir_.secret().get_Hash());
	if(fs::exists(hash_txt)) {
		ifs.open(hash_txt, std::ios_base::in);
		std::string hexhash_file;
		ifs >> hexhash_file;
		if(hexhash_file != hexhash_conf) wipe();
		ifs.close();
	}
	ifs.open(hash_txt, std::ios_base::out | std::ios_base::trunc);
	ifs << hexhash_conf;
}

bool Index::have_meta(const Meta::PathRevision& path_revision) noexcept {
	try {
		get_meta(path_revision);
	}catch(AbstractFolder::no_such_meta& e){
		return false;
	}
	return true;
}

SignedMeta Index::get_meta(const Meta::PathRevision& path_revision) {
	auto smeta = get_meta(path_revision.path_id_);
	if(smeta.meta().revision() == path_revision.revision_)
		return smeta;
	else throw AbstractFolder::no_such_meta();
}

/* Meta manipulators */

void Index::put_meta(const SignedMeta& signed_meta, bool fully_assembled) {
	std::ostringstream transaction_name; transaction_name << "put_Meta_" << std::this_thread::get_id();
	SQLiteSavepoint raii_transaction(*db_, transaction_name.str()); // Begin transaction

	db_->exec("INSERT OR REPLACE INTO meta (path_id, meta, signature, type, assembled) VALUES (:path_id, :meta, :signature, :type, :assembled);", {
			{":path_id", signed_meta.meta().path_id()},
			{":meta", signed_meta.raw_meta()},
			{":signature", signed_meta.signature()},
			{":type", (uint64_t)signed_meta.meta().meta_type()},
			{":assembled", (uint64_t)fully_assembled}
	});

	uint64_t offset = 0;
	for(auto chunk : signed_meta.meta().chunks()){
		db_->exec("INSERT OR IGNORE INTO chunk (ct_hash, size, iv) VALUES (:ct_hash, :size, :iv);", {
				{":ct_hash", chunk.ct_hash},
				{":size", (uint64_t)chunk.size},
				{":iv", chunk.iv}
		});

		db_->exec("INSERT OR REPLACE INTO openfs (ct_hash, path_id, [offset], assembled) VALUES (:ct_hash, :path_id, :offset, :assembled);", {
				{":ct_hash", chunk.ct_hash},
				{":path_id", signed_meta.meta().path_id()},
				{":offset", (uint64_t)offset},
				{":assembled", (uint64_t)fully_assembled}
		});

		offset += chunk.size;
	}

	raii_transaction.commit();  // End transaction

	if(fully_assembled)
		log_->debug() << log_tag() << "Added fully assembled Meta of " << dir_.path_id_readable(signed_meta.meta().path_id()) << " t:" << signed_meta.meta().meta_type();
	else
		log_->debug() << log_tag() << "Added Meta of " << dir_.path_id_readable(signed_meta.meta().path_id()) << " t:" << signed_meta.meta().meta_type();

	client_.network_ios().dispatch([=](){
		new_meta_signal(signed_meta);
		if(!fully_assembled)
			assemble_meta_signal(signed_meta.meta());
	});
}

std::list<SignedMeta> Index::get_meta(std::string sql, std::map<std::string, SQLValue> values){
	std::list<SignedMeta> result_list;
	for(auto row : db_->exec(sql, values))
		result_list.push_back(SignedMeta(row[0], row[1], dir_.secret()));
	return result_list;
}
SignedMeta Index::get_meta(const blob& path_id){
	auto meta_list = get_meta("SELECT meta, signature FROM meta WHERE path_id=:path_id LIMIT 1", {
		{":path_id", path_id}
	});

	if(meta_list.empty()) throw AbstractFolder::no_such_meta();
	return *meta_list.begin();
}
std::list<SignedMeta> Index::get_meta(){
	return get_meta("SELECT meta, signature FROM meta");
}

std::list<SignedMeta> Index::get_existing_meta() {
	return get_meta("SELECT meta, signature FROM meta WHERE (type<>255)=1 AND assembled=1;");
}

std::list<SignedMeta> Index::get_incomplete_meta() {
	return get_meta("SELECT meta, signature FROM meta WHERE (type<>255)=1 AND assembled=0;");
}

bool Index::put_allowed(const Meta::PathRevision& path_revision) noexcept {
	try {
		return get_meta(path_revision.path_id_).meta().revision() < path_revision.revision_;
	}catch(AbstractFolder::no_such_meta& e){
		return true;
	}
}

/* Block getter */

uint32_t Index::get_chunk_size(const blob& ct_hash) {
	auto sql_result = db_->exec("SELECT size FROM chunk WHERE ct_hash=:ct_hash", {
			{":ct_hash", ct_hash}
	});

	if(sql_result.have_rows())
		return (uint32_t)sql_result.begin()->at(0).as_uint();
	throw AbstractFolder::no_such_chunk();
}

std::list<SignedMeta> Index::containing_chunk(const blob& ct_hash) {
	return get_meta("SELECT meta.meta, meta.signature FROM meta JOIN openfs ON meta.path_id=openfs.path_id WHERE openfs.ct_hash=:ct_hash",
		{{":ct_hash", ct_hash}});
}

void Index::wipe() {
	SQLiteSavepoint savepoint(*db_, "Index::wipe");
	db_->exec("DELETE FROM meta");
	db_->exec("DELETE FROM chunk");
	db_->exec("DELETE FROM openfs");
	savepoint.commit();
	db_->exec("VACUUM");
}

Index::status_t Index::get_status() {
	auto sql_result = db_->exec("SELECT COUNT(*) FROM meta WHERE type=0 "
		"UNION ALL "
		"SELECT COUNT(*) FROM meta WHERE type=1 "
		"UNION ALL "
		"SELECT COUNT(*) FROM meta WHERE type=2 "
		"UNION ALL "
		"SELECT COUNT(*) FROM meta WHERE type=255");

	auto it = sql_result.begin();

	status_t s;
	s.file_entries = it[0].as_uint();
	++it;
	s.directory_entries = it[0].as_uint();
	++it;
	s.symlink_entries = it[0].as_uint();
	++it;
	s.deleted_entries = it[0].as_uint();
	return s;
}

} /* namespace librevault */
