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

#include <util/ffi.h>

#include <QFile>

#include "control/FolderParams.h"
#include "control/StateCollector.h"
#include "folder/meta/MetaStorage.h"
#include "util/readable.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

namespace librevault {

Index::Index(const FolderParams& params, StateCollector* state_collector, QObject* parent)
    : QObject(parent), params_(params), state_collector_(state_collector), index_(bridge::index_new((params_.system_path + "/librevault.db").toStdString())) {

  index_->c_migrate();

  auto db_filepath = params_.system_path + "/librevault.db";

  if (QFile::exists(db_filepath))
    LOGD("Opening SQLite3 DB:" << db_filepath);
  else
    LOGD("Creating new SQLite3 DB:" << db_filepath);
  db_ = std::make_unique<SQLiteDB>(db_filepath);
  db_->exec("PRAGMA foreign_keys = ON;");

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
  QJsonObject signed_meta_str{
    {"meta", QString::fromLatin1(signed_meta.meta().serialize().toBase64())},
    {"signature", QString::fromLatin1(signed_meta.signature().toBase64())}
  };
  index_->c_put_meta(QString::fromUtf8(QJsonDocument(signed_meta_str).toJson()).toStdString(), fully_assembled);

  emit metaAdded(signed_meta);
  if (!fully_assembled) emit metaAddedExternal(signed_meta);

  notifyState();
}

QList<SignedMeta> unwrap_rust(const rust::Vec<uint8_t>& data, const Secret& secret) {
  QList<SignedMeta> result_list;

  auto json = QJsonDocument::fromJson(from_vec(data)).object();
  auto metas = json["metas"].toArray();
  for (auto meta_val: metas) {
    auto meta_obj = meta_val.toObject();
    auto meta = QByteArray::fromBase64(meta_obj["meta"].toString().toLatin1());
    auto signature = QByteArray::fromBase64(meta_obj["signature"].toString().toLatin1());

    result_list << SignedMeta(meta, signature, secret);
  }

  return result_list;
}

SignedMeta Index::getMeta(const QByteArray& path_id) {
  try {
    auto meta_list = unwrap_rust(index_->c_get_meta_by_path_id(to_slice(path_id)), params_.secret);
    if (meta_list.empty()) throw MetaStorage::MetaNotFound();
    return meta_list.first();
  }catch (const std::exception& e) {
    throw MetaStorage::MetaNotFound();
  }
}
QList<SignedMeta> Index::getMeta() {
  try {
    return unwrap_rust(index_->c_get_meta_all(), params_.secret);
  }catch (const std::exception& e) {
    throw MetaStorage::MetaNotFound();
  }
}

QList<SignedMeta> Index::getExistingMeta() {
  try {
    return unwrap_rust(index_->c_get_meta_assembled(true), params_.secret);
  }catch (const std::exception& e) {
    throw MetaStorage::MetaNotFound();
  }
}

QList<SignedMeta> Index::getIncompleteMeta() {
  try {
    return unwrap_rust(index_->c_get_meta_assembled(false), params_.secret);
  }catch (const std::exception& e) {
    throw MetaStorage::MetaNotFound();
  }
}

bool Index::putAllowed(const Meta::PathRevision& path_revision) noexcept {
  try {
    return getMeta(path_revision.path_id_).meta().revision() < path_revision.revision_;
  } catch (MetaStorage::MetaNotFound& e) {
    return true;
  }
}

void Index::setAssembled(const QByteArray& path_id) {
  index_->set_assembled(to_slice(path_id));
}

bool Index::isAssembledChunk(const QByteArray& ct_hash) {
  return index_->is_chunk_assembled(to_slice(ct_hash));
}

QPair<quint32, QByteArray> Index::getChunkSizeIv(const QByteArray& ct_hash) {
  for (auto row :
       db_->exec("SELECT size, iv FROM chunk WHERE ct_hash=:ct_hash", {{":ct_hash", ct_hash}})) {
    return qMakePair(row[0].toUInt(), row[1].toByteArray());
  }
  throw MetaStorage::MetaNotFound();
};

QList<SignedMeta> Index::containingChunk(const QByteArray& ct_hash) {
  try {
    return unwrap_rust(index_->c_get_meta_with_chunk(to_slice(ct_hash)), params_.secret);
  }catch (const std::exception& e) {
    throw MetaStorage::MetaNotFound();
  }
}

void Index::wipe() {
  index_->wipe();
}

void Index::notifyState() {
  QJsonObject entries;
  for (auto row : db_->exec("SELECT type, COUNT(*) AS entries FROM meta GROUP BY type"))
    entries[QString::number(row[0].toUInt())] = (double)row[1].toUInt();
  state_collector_->folder_state_set(params_.secret.get_Hash(), "index", entries);
}

}  // namespace librevault
