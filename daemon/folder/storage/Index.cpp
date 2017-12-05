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
#include "config/FolderSettings.h"
#include "util/blob.h"
#include <QFile>
#include <ChunkInfo.h>
#include <QLoggingCategory>

namespace librevault {

Q_LOGGING_CATEGORY(log_index, "folder.storage.index")

Index::Index(const models::FolderSettings& params, QObject* parent) : QObject(parent), params_(params) {
	auto db_filepath = params_.effectiveSystemPath() + "/librevault.db";

	if(QFile::exists(db_filepath))
		qCDebug(log_index) << "Opening SQLite3 DB:" << db_filepath;
	else
		qCDebug(log_index) << "Creating new SQLite3 DB:" << db_filepath;
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
	QFile hash_file(params_.effectiveSystemPath() + "/hash.txt");
	QByteArray hexhash_conf = params_.secret.folderid();
	if(hash_file.exists()) {
		hash_file.open(QIODevice::ReadOnly);
		if(hash_file.readAll().toLower() != hexhash_conf.toLower()) wipe();
		hash_file.close();
	}
	hash_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
	hash_file.write(hexhash_conf);
	hash_file.close();
}

bool Index::haveMeta(const MetaInfo::PathRevision& path_revision) noexcept {
	try {
		getMeta(path_revision);
	}catch(NoSuchMeta& e){
		return false;
	}
	return true;
}

SignedMeta Index::getMeta(const MetaInfo::PathRevision& path_revision) {
	auto smeta = getMeta(path_revision.path_keyed_hash_);
	if(smeta.metaInfo().revision() == path_revision.revision_)
		return smeta;
	else throw Index::NoSuchMeta();
}

/* Meta manipulators */

void Index::putMeta(const SignedMeta& signed_meta, bool fully_assembled) {
	qsrand(time(nullptr));
	QString transaction_name = QStringLiteral("put_Meta_%1").arg(qrand());
	SQLiteSavepoint raii_transaction(*db_, transaction_name.toStdString()); // Begin transaction

	db_->exec("INSERT OR REPLACE INTO meta (path_id, meta, signature, type, assembled) VALUES (:path_id, :meta, :signature, :type, :assembled);", {
			{":path_id", convertBlob(signed_meta.metaInfo().pathKeyedHash())},
			{":meta", convertBlob(signed_meta.rawMetaInfo())},
			{":signature", convertBlob(signed_meta.signature())},
			{":type", (uint64_t) signed_meta.metaInfo().kind()},
			{":assembled", (uint64_t)fully_assembled}
	});

	uint64_t offset = 0;
	for(const auto& chunk : signed_meta.metaInfo().chunks()){
		db_->exec("INSERT OR IGNORE INTO chunk (ct_hash, size, iv) VALUES (:ct_hash, :size, :iv);", {
				{":ct_hash", convertBlob(chunk.ctHash())},
				{":size", (uint64_t)chunk.size()},
				{":iv", convertBlob(chunk.iv())}
		});

		db_->exec("INSERT OR REPLACE INTO openfs (ct_hash, path_id, [offset], assembled) VALUES (:ct_hash, :path_id, :offset, :assembled);", {
				{":ct_hash", convertBlob(chunk.ctHash())},
				{":path_id", convertBlob(signed_meta.metaInfo().pathKeyedHash())},
				{":offset", (uint64_t)offset},
				{":assembled", (uint64_t)fully_assembled}
		});

		offset += chunk.size();
	}

	raii_transaction.commit();  // End transaction

	if(fully_assembled)
		qCDebug(log_index) << "Added fully assembled Meta of" << signed_meta.metaInfo().pathKeyedHash().toHex() << "t:" << signed_meta.metaInfo().kind();
	else
		qCDebug(log_index) << "Added Meta of" << signed_meta.metaInfo().pathKeyedHash().toHex() << "t:" << signed_meta.metaInfo().kind();

	emit metaAdded(signed_meta);
	if(!fully_assembled)
		emit metaAddedExternal(signed_meta);
}

QList<SignedMeta> Index::getMeta(const std::string& sql, const std::map<std::string, SQLValue>& values){
	QList<SignedMeta> result_list;
	for(auto row : db_->exec(sql, values))
		result_list << SignedMeta(convertBlob(row[0]), convertBlob(row[1]));
	return result_list;
}
SignedMeta Index::getMeta(const QByteArray& path_id){
	auto meta_list = getMeta("SELECT meta, signature FROM meta WHERE path_id=:path_id LIMIT 1", {
		{":path_id", convertBlob(path_id)}
	});

	if(meta_list.empty()) throw Index::NoSuchMeta();
	return *meta_list.begin();
}
QList<SignedMeta> Index::getMeta(){
	return getMeta(std::string("SELECT meta, signature FROM meta"));
}

QList<SignedMeta> Index::getExistingMeta() {
	return getMeta(std::string("SELECT meta, signature FROM meta WHERE (type<>255)=1 AND assembled=1;"));
}

QList<SignedMeta> Index::getIncompleteMeta() {
	return getMeta(std::string("SELECT meta, signature FROM meta WHERE (type<>255)=1 AND assembled=0;"));
}

bool Index::putAllowed(const MetaInfo::PathRevision& path_revision) noexcept {
	try {
		return getMeta(path_revision.path_keyed_hash_).metaInfo().revision() < path_revision.revision_;
	}catch(NoSuchMeta& e){
		return true;
	}
}

void Index::setAssembled(const QByteArray& path_id) {
	db_->exec("UPDATE meta SET assembled=1 WHERE path_id=:path_id", {{":path_id", convertBlob(path_id)}});
	db_->exec("UPDATE openfs SET assembled=1 WHERE path_id=:path_id", {{":path_id", convertBlob(path_id)}});
}

bool Index::isChunkAssembled(const QByteArray& ct_hash) {
	auto sql_result = db_->exec("SELECT assembled FROM openfs WHERE ct_hash=:ct_hash AND openfs.assembled=1 LIMIT 1", {
		{":ct_hash", convertBlob(ct_hash)}
	});
	return sql_result.have_rows();
}

QPair<quint32, QByteArray> Index::getChunkSizeIv(const QByteArray& ct_hash) {
	for(auto row : db_->exec("SELECT size, iv FROM chunk WHERE ct_hash=:ct_hash", {{":ct_hash", convertBlob(ct_hash)}})) {
		return qMakePair(row[0].as_uint(), convertBlob(row[1].as_blob()));
	}
	throw Index::NoSuchMeta();
};

QList<SignedMeta> Index::containingChunk(const QByteArray& ct_hash) {
	return getMeta("SELECT meta.meta, meta.signature FROM meta JOIN openfs ON meta.path_id=openfs.path_id WHERE openfs.ct_hash=:ct_hash",
		{{":ct_hash", convertBlob(ct_hash)}});
}

void Index::wipe() {
	SQLiteSavepoint savepoint(*db_, "Index::wipe");
	db_->exec("DELETE FROM meta");
	db_->exec("DELETE FROM chunk");
	db_->exec("DELETE FROM openfs");
	savepoint.commit();
	db_->exec("VACUUM");
}

} /* namespace librevault */
