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
#include "IndexerWorker.h"

#include <QFile>
#include <boost/filesystem.hpp>

#include "MetaStorage.h"
#include "control/FolderParams.h"
#include "crypto/AES_CBC.h"
#include "crypto/KMAC-SHA3.h"
#include "folder/IgnoreList.h"
#include "folder/PathNormalizer.h"
#include "util/human_size.h"
#include "util/conv_fspath.h"
#include <librevault_util/src/indexer.rs.h>
#ifdef Q_OS_UNIX
#include <src/common/Meta_s.pb.h>
#include <sys/stat.h>
#include <util/ffi.h>

#include <librevaultrs.hpp>
#endif
#ifdef Q_OS_WIN
#include <windows.h>
#endif

Q_DECLARE_LOGGING_CATEGORY(log_indexer)

namespace librevault {

Meta::Chunk convert_chunk(const QByteArray& chunk) {
  auto chunk_s = serialization::Meta_FileMetadata_Chunk();
  chunk_s.ParseFromArray(chunk.data(), chunk.size());
  return {QByteArray::fromStdString(chunk_s.ct_hash()), chunk_s.size(), QByteArray::fromStdString(chunk_s.iv()), QByteArray::fromStdString(chunk_s.pt_hmac())};
}

IndexerWorker::IndexerWorker(QString abspath, const FolderParams& params, MetaStorage* meta_storage,
                             IgnoreList* ignore_list, PathNormalizer* path_normalizer, QObject* parent)
    : QObject(parent),
      abspath_(abspath),
      params_(params),
      meta_storage_(meta_storage),
      ignore_list_(ignore_list),
      path_normalizer_(path_normalizer),
      secret_(params.secret),
      active_(true) {}

IndexerWorker::~IndexerWorker() {}

void IndexerWorker::run() noexcept {
  QByteArray normpath = path_normalizer_->normalizePath(abspath_);
  qCDebug(log_indexer) << "Started indexing:" << normpath;

  try {
    if (ignore_list_->isIgnored(normpath)) throw AbortIndex("File is ignored");

    auto path_id = Meta::make_path_id(normpath, secret_);

    try {
      old_smeta_ = meta_storage_->getMeta(path_id);
      old_meta_ = old_smeta_.meta();

      auto new_mtime = boost::filesystem::last_write_time(conv_fspath(abspath_));

      if (new_mtime == old_meta_.mtime()) {
        throw AbortIndex("Modification time is not changed");
      }else {
        qCDebug(log_indexer) << "Old mtime: " << old_meta_.mtime() << " New mtime: " << new_mtime;
      }
    } catch (boost::filesystem::filesystem_error& e) {
      qCDebug(log_indexer) << "Filesystem Error";
    } catch (MetaStorage::MetaNotFound& e) {
      qCDebug(log_indexer) << "Meta for path_id: " << path_id << " not found";
    } catch (Meta::error& e) {
      qCDebug(log_indexer) << "Meta in DB is inconsistent, trying to reindex:" << e.what();
    }

    QElapsedTimer timer_;
    timer_.start();  // Starting timer
    make_Meta();     // Actual indexing
    qreal time_spent = qreal(timer_.elapsed()) / 1000;
    qreal bandwidth = qreal(new_smeta_.meta().size()) / time_spent;

    qCDebug(log_indexer) << "Updated index entry in" << time_spent << "s (" << human_bandwidth(bandwidth) << ")"
                         << "Path=" << abspath_ << "Rev=" << new_smeta_.meta().revision()
                         << "Chk=" << new_smeta_.meta().chunks().size();

    emit metaCreated(new_smeta_);
  } catch (std::runtime_error& e) {
    emit metaFailed(e.what());
  }
}

/* Actual indexing process */
void IndexerWorker::make_Meta() {
  QString abspath = abspath_;
  QByteArray normpath = path_normalizer_->normalizePath(abspath);

  // LOGD("make_Meta(" << normpath.toStdString() << ")");

  new_meta_.set_path(normpath, secret_);  // sets path_id, encrypted_path and encrypted_path_iv

  new_meta_.set_meta_type(get_type());  // Type

  if (!old_smeta_ && new_meta_.meta_type() == Meta::DELETED)
    throw AbortIndex("Old Meta is not in the index, new Meta is DELETED");

  if (old_meta_.meta_type() == Meta::DIRECTORY && new_meta_.meta_type() == Meta::DIRECTORY)
    throw AbortIndex("Old Meta is DIRECTORY, new Meta is DIRECTORY");

  if (old_meta_.meta_type() == Meta::DELETED && new_meta_.meta_type() == Meta::DELETED)
    throw AbortIndex("Old Meta is DELETED, new Meta is DELETED");

  if (new_meta_.meta_type() == Meta::FILE) update_chunks();

  if (new_meta_.meta_type() == Meta::SYMLINK) {
    new_meta_.set_symlink_path(boost::filesystem::read_symlink(abspath_.toStdWString()).generic_string(), secret_);
  }

  // FSAttrib
  if (new_meta_.meta_type() != Meta::DELETED)
    update_fsattrib();  // Platform-dependent attributes (windows attrib, uid, gid, mode)

  // Revision
  new_meta_.set_revision(std::time(nullptr));  // Meta is ready. Assigning timestamp.

  new_smeta_ = SignedMeta(new_meta_, secret_);
}

Meta::Type IndexerWorker::get_type() {
  namespace fs = boost::filesystem;
  fs::file_status file_status = params_.preserve_symlinks
                                    ? fs::symlink_status(conv_fspath(abspath_))
                                    : fs::status(conv_fspath(abspath_));  // Preserves symlinks if such option is set.

  switch (file_status.type()) {
    case fs::regular_file:
      return Meta::FILE;
    case fs::directory_file:
      return Meta::DIRECTORY;
    case fs::symlink_file:
      return Meta::SYMLINK;
    case fs::file_not_found:
      return Meta::DELETED;
    default:
      throw AbortIndex(
          "File type is unsuitable for indexing. Only Files, Directories and Symbolic links are supported");
  }
}

void IndexerWorker::update_fsattrib() {
  boost::filesystem::path babspath(abspath_.toStdWString());

  // First, preserve old values of attributes
  new_meta_.set_windows_attrib(old_meta_.windows_attrib());
  new_meta_.set_mode(old_meta_.mode());
  new_meta_.set_uid(old_meta_.uid());
  new_meta_.set_gid(old_meta_.gid());

  if (new_meta_.meta_type() != Meta::SYMLINK)
    new_meta_.set_mtime(boost::filesystem::last_write_time(babspath));  // File/directory modification time
  else {
    // TODO: make alternative function for symlinks. Use boost::filesystem::last_write_time as an example. lstat for
    // Unix and GetFileAttributesEx for Windows.
  }

  // Then, write new values of attributes (if enabled in config)
#if defined(Q_OS_WIN)
  if (params_.preserve_windows_attrib) {
    new_meta_.set_windows_attrib(GetFileAttributes(
        babspath.native()
            .c_str()));  // Windows attributes (I don't have Windows now to test it), this code is stub for now.
  }
#elif defined(Q_OS_UNIX)
  if (params_.preserve_unix_attrib) {
    struct stat stat_buf;
    int stat_err = 0;
    if (params_.preserve_symlinks)
      stat_err = lstat(babspath.c_str(), &stat_buf);
    else
      stat_err = stat(babspath.c_str(), &stat_buf);
    if (stat_err == 0) {
      new_meta_.set_mode(stat_buf.st_mode);
      new_meta_.set_uid(stat_buf.st_uid);
      new_meta_.set_gid(stat_buf.st_gid);
    }
  }
#endif
}

void IndexerWorker::update_chunks() {
  auto d = QJsonDocument::fromJson(from_vec(c_make_chunks(abspath_.toStdString(), secret_.string().toStdString())));

  QVector<Meta::Chunk> chunks;
  for(auto chunk_b64 : d.array()) {
    auto chunk_s = QByteArray::fromBase64(chunk_b64.toString().toLatin1());
    chunks += convert_chunk(chunk_s);
  }

  new_meta_.set_chunks(chunks);
}

}  // namespace librevault
