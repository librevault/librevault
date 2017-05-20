/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "Index.h"
#include "control/FolderParams.h"
#include "control/StateCollector.h"
#include "folder/meta/MetaStorage.h"
#include "util/readable.h"
#include <QFile>

namespace librevault {

Index::Index(const FolderParams& params, StateCollector* state_collector, QObject* parent) : QObject(parent), params_(params), state_collector_(state_collector) {
	auto db_filepath = params_.system_path + "/librevault.db";

	if(QFile::exists(db_filepath))
		LOGD("Opening SQLite3 DB:" << db_filepath);
	else
		LOGD("Creating new SQLite3 DB:" << db_filepath);
	db_ = std::make_unique<SQLiteDB>(db_filepath.toStdString());
	db_->exec("PRAGMA foreign_keys = ON;");

	/* TABLE meta */
	db_->exec("CREATE TABLE IF NOT EXISTS meta (path_id BLOB PRIMARY KEY NOT NULL, meta BLOB NOT NULL, signature BLOB NOT NULL, type INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");
	db_->exec("CREATE INDEX IF NOT EXISTS meta_type_idx ON meta (type);");   // For making "COUNT(*) ... WHERE type=x" way faster
	db_->exec("CREATE INDEX IF NOT EXISTS meta_not_deleted_idx ON meta(type<>255);");   // For faster Index::getExistingMeta

	/* TABLE chunk */
	db_->exec("CREATE TABLE IF NOT EXISTS chunk (ct_hash BLOB NOT NULL PRIMARY KEY, size INTEGER NOT NULL, iv BLOB NOT NULL);");

	/* TABLE openfs */
	db_->exec("CREATE TABLE IF NOT EXISTS openfs (ct_hash BLOB NOT NULL REFERENCES chunk (ct_hash) ON DELETE CASCADE ON UPDATE CASCADE, path_id BLOB NOT NULL REFERENCES meta (path_id) ON DELETE CASCADE ON UPDATE CASCADE, [offset] INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");
	db_->exec("CREATE INDEX IF NOT EXISTS openfs_assembled_idx ON openfs (ct_hash, assembled) WHERE assembled = 1;");    // For faster OpenStorage::have_chunk
	db_->exec("CREATE INDEX IF NOT EXISTS openfs_path_id_fki ON openfs (path_id);");    // For faster AssemblerQueue::assemble_file
	db_->exec("CREATE IF NOT EXISTS INDEX openfs_ct_hash_fki ON openfs (ct_hash);");    // For faster Index::containingChunk
	//db_->exec("CREATE TRIGGER IF NOT EXISTS chunk_deleter AFTER DELETE ON openfs BEGIN DELETE FROM chunk WHERE ct_hash NOT IN (SELECT ct_hash FROM openfs); END;");   // Damn, there are more problems with this trigger than profit from it. Anyway, we can add it anytime later.

	/* Create a special hash-file */
	QFile hash_file(params_.system_path + "/hash.txt");
	QByteArray hexhash_conf = params_.secret.getHash();
	if(hash_file.exists()) {
		hash_file.open(QIODevice::ReadOnly);
		if(hash_file.readAll().toLower() != hexhash_conf.toLower()) wipe();
		hash_file.close();
	}
	hash_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
	hash_file.write(hexhash_conf);
	hash_file.close();

	notifyState();
}

bool Index::haveMeta(const Meta::PathRevision& path_revision) noexcept {
	try {
		getMeta(path_revision);
	}catch(MetaStorage::no_such_meta& e){
		return false;
	}
	return true;
}

SignedMeta Index::getMeta(const Meta::PathRevision& path_revision) {
	auto smeta = getMeta(path_revision.path_id_);
	if(smeta.meta().revision() == path_revision.revision_)
		return smeta;
	else throw MetaStorage::no_such_meta();
}

/* Meta manipulators */

void Index::putMeta(const SignedMeta& signed_meta, bool fully_assembled) {
	LOGFUNC();
	qsrand(time(nullptr));
	QString transaction_name = QStringLiteral("put_Meta_%1").arg(qrand());
	SQLiteSavepoint raii_transaction(*db_, transaction_name.toStdString()); // Begin transaction

	db_->exec("INSERT OR REPLACE INTO meta (path_id, meta, signature, type, assembled) VALUES (:path_id, :meta, :signature, :type, :assembled);", {
			{":path_id", signed_meta.meta().path_id()},
			{":meta", conv_bytearray(signed_meta.raw_meta())},
			{":signature", conv_bytearray(signed_meta.signature())},
			{":type", (uint64_t)signed_meta.meta().meta_type()},
			{":assembled", (uint64_t)fully_assembled}
	});

	uint64_t offset = 0;
	for(auto chunk : signed_meta.meta().chunks()){
		db_->exec("INSERT OR IGNORE INTO chunk (ct_hash, size, iv) VALUES (:ct_hash, :size, :iv);", {
				{":ct_hash", conv_bytearray(chunk.ct_hash)},
				{":size", (uint64_t)chunk.size},
				{":iv", conv_bytearray(chunk.iv)}
		});

		db_->exec("INSERT OR REPLACE INTO openfs (ct_hash, path_id, [offset], assembled) VALUES (:ct_hash, :path_id, :offset, :assembled);", {
				{":ct_hash", conv_bytearray(chunk.ct_hash)},
				{":path_id", signed_meta.meta().path_id()},
				{":offset", (uint64_t)offset},
				{":assembled", (uint64_t)fully_assembled}
		});

		offset += chunk.size;
	}

	raii_transaction.commit();  // End transaction

	if(fully_assembled)
		LOGD("Added fully assembled Meta of " << path_id_readable(signed_meta.meta().path_id()) << " t:" << signed_meta.meta().meta_type());
	else
		LOGD("Added Meta of " << path_id_readable(signed_meta.meta().path_id()) << " t:" << signed_meta.meta().meta_type());

	emit metaAdded(signed_meta);
	if(!fully_assembled)
		emit metaAddedExternal(signed_meta);

	notifyState();
}

QList<SignedMeta> Index::getMeta(const std::string& sql, const std::map<std::string, SQLValue>& values){
	QList<SignedMeta> result_list;
	for(auto row : db_->exec(sql, values))
		result_list << SignedMeta(conv_bytearray(row[0]), conv_bytearray(row[1]));
	return result_list;
}
SignedMeta Index::getMeta(const blob& path_id){
	auto meta_list = getMeta("SELECT meta, signature FROM meta WHERE path_id=:path_id LIMIT 1", {
		{":path_id", path_id}
	});

	if(meta_list.empty()) throw MetaStorage::no_such_meta();
	return *meta_list.begin();
}
QList<SignedMeta> Index::getMeta(){
	return getMeta("SELECT meta, signature FROM meta");
}

QList<SignedMeta> Index::getExistingMeta() {
	return getMeta("SELECT meta, signature FROM meta WHERE (type<>255)=1 AND assembled=1;");
}

QList<SignedMeta> Index::getIncompleteMeta() {
	return getMeta("SELECT meta, signature FROM meta WHERE (type<>255)=1 AND assembled=0;");
}

bool Index::putAllowed(const Meta::PathRevision& path_revision) noexcept {
	try {
		return getMeta(path_revision.path_id_).meta().revision() < path_revision.revision_;
	}catch(MetaStorage::no_such_meta& e){
		return true;
	}
}

void Index::setAssembled(blob path_id) {
	db_->exec("UPDATE meta SET assembled=1 WHERE path_id=:path_id", {{":path_id", path_id}});
	db_->exec("UPDATE openfs SET assembled=1 WHERE path_id=:path_id", {{":path_id", path_id}});
}

bool Index::isAssembledChunk(blob ct_hash) {
	auto sql_result = db_->exec("SELECT assembled FROM openfs WHERE ct_hash=:ct_hash AND openfs.assembled=1 LIMIT 1", {
		{":ct_hash", ct_hash}
	});
	return sql_result.have_rows();
}

QPair<quint32, QByteArray> Index::getChunkSizeIv(blob ct_hash) {
	for(auto row : db_->exec("SELECT size, iv FROM chunk WHERE ct_hash=:ct_hash", {{":ct_hash", ct_hash}})) {
		return qMakePair(row[0].as_uint(), conv_bytearray(row[1].as_blob()));
	}
	throw MetaStorage::no_such_meta();
};

QList<SignedMeta> Index::containingChunk(const blob& ct_hash) {
	return getMeta("SELECT meta.meta, meta.signature FROM meta JOIN openfs ON meta.path_id=openfs.path_id WHERE openfs.ct_hash=:ct_hash",
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

void Index::notifyState() {
	QJsonObject entries;
	for(auto row : db_->exec("SELECT type, COUNT(*) AS entries FROM meta GROUP BY type")) {
		entries[QString::number(row[0].as_uint())] = (double)row[1].as_uint();
	}
	state_collector_->folder_state_set(params_.secret.getHash(), "index", entries);
}

} /* namespace librevault */
