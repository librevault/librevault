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
 */
#include "Index.h"

#include <util/conv_fspath.h>

#include <QFile>

#include "control/FolderParams.h"
#include "control/StateCollector.h"
#include "folder/meta/MetaStorage.h"
#include "util/readable.h"

namespace librevault {

Index::Index(const FolderParams& params, StateCollector* state_collector, QObject* parent)
    : QObject(parent), params_(params), state_collector_(state_collector) {
  auto db_filepath = params_.system_path + "/librevault.db";

  if (QFile::exists(db_filepath))
    LOGD("Opening SQLite3 DB:" << db_filepath);
  else
    LOGD("Creating new SQLite3 DB:" << db_filepath);
  db_ = std::make_unique<SQLiteDB>(db_filepath);
  db_->exec("PRAGMA foreign_keys = ON;");

  /* TABLE meta */
  db_->exec(
      "CREATE TABLE IF NOT EXISTS meta (path_id BLOB PRIMARY KEY NOT NULL, meta BLOB NOT NULL, signature BLOB NOT "
      "NULL, type INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");
  db_->exec(
      "CREATE INDEX IF NOT EXISTS meta_type_idx ON meta (type);");  // For making "COUNT(*) ... WHERE type=x" way faster
  db_->exec(
      "CREATE INDEX IF NOT EXISTS meta_not_deleted_idx ON meta(type<>255);");  // For faster Index::getExistingMeta

  /* TABLE chunk */
  db_->exec(
      "CREATE TABLE IF NOT EXISTS chunk (ct_hash BLOB NOT NULL PRIMARY KEY, size INTEGER NOT NULL, iv BLOB NOT NULL);");

  /* TABLE openfs */
  db_->exec(
      "CREATE TABLE IF NOT EXISTS openfs (ct_hash BLOB NOT NULL REFERENCES chunk (ct_hash) ON DELETE CASCADE ON UPDATE "
      "CASCADE, path_id BLOB NOT NULL REFERENCES meta (path_id) ON DELETE CASCADE ON UPDATE CASCADE, [offset] INTEGER "
      "NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);");
  db_->exec(
      "CREATE INDEX IF NOT EXISTS openfs_assembled_idx ON openfs (ct_hash, assembled) WHERE assembled = 1;");  // For
                                                                                                               // faster
                                                                                                               // OpenStorage::have_chunk
  db_->exec("CREATE INDEX IF NOT EXISTS openfs_path_id_fki ON openfs (path_id);");  // For faster
                                                                                    // AssemblerQueue::assemble_file
  db_->exec("CREATE IF NOT EXISTS INDEX openfs_ct_hash_fki ON openfs (ct_hash);");  // For faster Index::containingChunk
  // db_->exec("CREATE TRIGGER IF NOT EXISTS chunk_deleter AFTER DELETE ON openfs BEGIN DELETE FROM chunk WHERE ct_hash
  // NOT IN (SELECT ct_hash FROM openfs); END;");   // Damn, there are more problems with this trigger than profit from
  // it. Anyway, we can add it anytime later.

  /* Create a special hash-file */
  QFile hash_file(params_.system_path + "/hash.txt");
  QByteArray hexhash_conf = params_.secret.get_Hash();
  if (hash_file.exists()) {
    hash_file.open(QIODevice::ReadOnly);
    if (hash_file.readAll().toLower() != hexhash_conf.toLower()) wipe();
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
  } catch (MetaStorage::MetaNotFound& e) {
    return false;
  }
  return true;
}

SignedMeta Index::getMeta(const Meta::PathRevision& path_revision) {
  auto smeta = getMeta(path_revision.path_id_);
  if (smeta.meta().revision() == path_revision.revision_)
    return smeta;
  else
    throw MetaStorage::MetaNotFound();
}

/* Meta manipulators */

void Index::putMeta(const SignedMeta& signed_meta, bool fully_assembled) {
  LOGFUNC();
  SQLiteSavepoint raii_transaction(*db_);  // Begin transaction

  db_->exec(
      "INSERT OR REPLACE INTO meta (path_id, meta, signature, type, assembled) VALUES (:path_id, :meta, :signature, "
      ":type, :assembled);",
      {{":path_id", signed_meta.meta().path_id()},
       {":meta", signed_meta.raw_meta()},
       {":signature", signed_meta.signature()},
       {":type", (uint64_t)signed_meta.meta().meta_type()},
       {":assembled", (uint64_t)fully_assembled}});

  uint64_t offset = 0;
  for (auto chunk : signed_meta.meta().chunks()) {
    db_->exec("INSERT OR IGNORE INTO chunk (ct_hash, size, iv) VALUES (:ct_hash, :size, :iv);",
              {{":ct_hash", chunk.ct_hash}, {":size", (uint64_t)chunk.size}, {":iv", chunk.iv}});

    db_->exec(
        "INSERT OR REPLACE INTO openfs (ct_hash, path_id, [offset], assembled) VALUES (:ct_hash, :path_id, :offset, "
        ":assembled);",
        {{":ct_hash", chunk.ct_hash},
         {":path_id", signed_meta.meta().path_id()},
         {":offset", (uint64_t)offset},
         {":assembled", (uint64_t)fully_assembled}});

    offset += chunk.size;
  }

  raii_transaction.commit();  // End transaction

  if (fully_assembled)
    LOGD("Added fully assembled Meta of " << path_id_readable(signed_meta.meta().path_id())
                                          << " t:" << signed_meta.meta().meta_type());
  else
    LOGD("Added Meta of " << path_id_readable(signed_meta.meta().path_id()) << " t:" << signed_meta.meta().meta_type());

  emit metaAdded(signed_meta);
  if (!fully_assembled) emit metaAddedExternal(signed_meta);

  notifyState();
}

QList<SignedMeta> Index::getMeta(const QString& sql, const std::map<QString, SQLValue>& values) {
  QList<SignedMeta> result_list;
  for (auto row : db_->exec(sql, values)) result_list << SignedMeta(row[0], row[1], params_.secret);
  return result_list;
}
SignedMeta Index::getMeta(const QByteArray& path_id) {
  auto meta_list = getMeta("SELECT meta, signature FROM meta WHERE path_id=:path_id LIMIT 1", {{":path_id", path_id}});

  if (meta_list.empty()) throw MetaStorage::MetaNotFound();
  return *meta_list.begin();
}
QList<SignedMeta> Index::getMeta() { return getMeta("SELECT meta, signature FROM meta", {}); }

QList<SignedMeta> Index::getExistingMeta() {
  return getMeta("SELECT meta, signature FROM meta WHERE (type<>255)=1 AND assembled=1;", {});
}

QList<SignedMeta> Index::getIncompleteMeta() {
  return getMeta("SELECT meta, signature FROM meta WHERE (type<>255)=1 AND assembled=0;", {});
}

bool Index::putAllowed(const Meta::PathRevision& path_revision) noexcept {
  try {
    return getMeta(path_revision.path_id_).meta().revision() < path_revision.revision_;
  } catch (MetaStorage::MetaNotFound& e) {
    return true;
  }
}

void Index::setAssembled(const QByteArray& path_id) {
  db_->exec("UPDATE meta SET assembled=1 WHERE path_id=:path_id", {{":path_id", path_id}});
  db_->exec("UPDATE openfs SET assembled=1 WHERE path_id=:path_id", {{":path_id", path_id}});
}

bool Index::isAssembledChunk(const QByteArray& ct_hash) {
  auto sql_result = db_->exec("SELECT assembled FROM openfs WHERE ct_hash=:ct_hash AND openfs.assembled=1 LIMIT 1",
                              {{":ct_hash", ct_hash}});
  return sql_result.have_rows();
}

QPair<quint32, QByteArray> Index::getChunkSizeIv(const QByteArray& ct_hash) {
  for (auto row :
       db_->exec("SELECT size, iv FROM chunk WHERE ct_hash=:ct_hash", {{":ct_hash", ct_hash}})) {
    return qMakePair(row[0].toUInt(), row[1].toByteArray());
  }
  throw MetaStorage::MetaNotFound();
};

QList<SignedMeta> Index::containingChunk(const QByteArray& ct_hash) {
  return getMeta(
      "SELECT meta.meta, meta.signature FROM meta JOIN openfs ON meta.path_id=openfs.path_id WHERE "
      "openfs.ct_hash=:ct_hash",
      {{":ct_hash", ct_hash}});
}

void Index::wipe() {
  SQLiteSavepoint savepoint(*db_);
  db_->exec("DELETE FROM meta");
  db_->exec("DELETE FROM chunk");
  db_->exec("DELETE FROM openfs");
  savepoint.commit();
  db_->exec("VACUUM");
}

void Index::notifyState() {
  QJsonObject entries;
  for (auto row : db_->exec("SELECT type, COUNT(*) AS entries FROM meta GROUP BY type"))
    entries[QString::number(row[0].toUInt())] = (double)row[1].toUInt();
  state_collector_->folder_state_set(params_.secret.get_Hash(), "index", entries);
}

}  // namespace librevault
