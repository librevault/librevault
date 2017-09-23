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
#include "ScanTask.h"
#include "AES_CBC.h"
#include "control/FolderParams.h"
#include "folder/IgnoreList.h"
#include "folder/storage/Index.h"
#include "human_size.h"
#include <ChunkInfo.h>
#include <PathNormalizer.h>
#include <QFile>
#include <boost/filesystem.hpp>
#include <Rabin.hpp>
#ifdef Q_OS_UNIX
#  include <sys/stat.h>
#endif
#ifdef Q_OS_WIN
#  include <windows.h>
#endif

Q_LOGGING_CATEGORY(log_indexer, "folder.storage.scanner")

namespace librevault {

ScanTask::ScanTask(QString abspath, const FolderParams& params, Index* index, IgnoreList* ignore_list, QObject* parent)
    : QueuedTask(SCAN, parent),
      abspath_(abspath),
      params_(params),
      index_(index),
      ignore_list_(ignore_list),
      secret_(params.secret) {}

ScanTask::~ScanTask() {}

void ScanTask::run() noexcept {
  QByteArray normpath = PathNormalizer::normalizePath(abspath_, params_.path);
  qCDebug(log_indexer) << "Started indexing:" << normpath;

  try {
    if (ignore_list_->isIgnored(normpath)) throw AbortIndex("File is ignored");

    try {
      old_smeta_ = index_->getMeta(MetaInfo::makePathId(normpath, secret_));
      old_meta_ = old_smeta_.metaInfo();
      if (boost::filesystem::last_write_time(abspath_.toStdString()) == old_meta_.mtime()) {
        throw AbortIndex("Modification time is not changed");
      }
    } catch (boost::filesystem::filesystem_error& e) {
    } catch (Index::NoSuchMeta& e) {
    } catch (MetaInfo::error& e) {
      qCDebug(log_indexer) << "Meta in DB is inconsistent, trying to reindex:" << e.what();
    }

    QElapsedTimer timer_;
    timer_.start();  // Starting timer
    makeMetaInfo();     // Actual indexing
    qreal time_spent = qreal(timer_.elapsed()) / 1000;
    qreal bandwidth = qreal(new_smeta_.metaInfo().size()) / time_spent;

    qCDebug(log_indexer) << "Updated index entry in" << time_spent << "s (" << human_bandwidth(bandwidth) << ")"
                         << "Path=" << abspath_
                         << "Rev=" << new_smeta_.metaInfo().timestamp().time_since_epoch().count()
                         << "Chk=" << new_smeta_.metaInfo().chunks().size();

    index_->putMeta(new_smeta_, true);
  } catch (const std::exception& e) {
    qCWarning(log_indexer) << "Skipping" << absolutePath() << "Reason:" << e.what();
  }
}

/* Actual indexing process */
void ScanTask::makeMetaInfo() {
  QByteArray normpath = PathNormalizer::normalizePath(abspath_, params_.path);

  // LOGD("make_Meta(" << normpath.toStdString() << ")");

  new_meta_.path(EncryptedData::fromPlaintext(
      normpath, secret_.encryptionKey(), generateRandomIV()));  // sets path_id, encrypted_path and encrypted_path_iv
  new_meta_.pathKeyedHash(MetaInfo::makePathId(normpath, secret_));

  new_meta_.kind(get_type());  // Kind

  if (!old_smeta_ && new_meta_.kind() == MetaInfo::DELETED)
    throw AbortIndex("Old Meta is not in the index, new Meta is DELETED");

  if (old_meta_.kind() == MetaInfo::DIRECTORY && new_meta_.kind() == MetaInfo::DIRECTORY)
    throw AbortIndex("Old Meta is DIRECTORY, new Meta is DIRECTORY");

  if (old_meta_.kind() == MetaInfo::DELETED && new_meta_.kind() == MetaInfo::DELETED)
    throw AbortIndex("Old Meta is DELETED, new Meta is DELETED");

  if (new_meta_.kind() == MetaInfo::FILE) update_chunks();

  if (new_meta_.kind() == MetaInfo::SYMLINK) {
    QByteArray symlink_target =
        QByteArray::fromStdString(boost::filesystem::read_symlink(abspath_.toStdWString()).generic_string());
    new_meta_.symlinkTarget(EncryptedData::fromPlaintext(symlink_target, secret_.encryptionKey(), generateRandomIV()));
  }

  // FSAttrib
  if (new_meta_.kind() != MetaInfo::DELETED)
    update_fsattrib();  // Platform-dependent attributes (windows attrib, uid, gid, mode)

  // Revision
  new_meta_.timestamp(std::chrono::system_clock::now());  // Meta is ready. Assigning timestamp.

  new_smeta_ = SignedMeta(new_meta_, secret_);
}

MetaInfo::Kind ScanTask::get_type() {
  QString abspath = abspath_;
  boost::filesystem::path babspath(abspath.toStdWString());

  boost::filesystem::file_status file_status =
      params_.preserve_symlinks ? boost::filesystem::symlink_status(babspath)
                                : boost::filesystem::status(babspath);  // Preserves symlinks if such option is set.

  switch (file_status.type()) {
    case boost::filesystem::regular_file:
      return MetaInfo::FILE;
    case boost::filesystem::directory_file:
      return MetaInfo::DIRECTORY;
    case boost::filesystem::symlink_file:
      return MetaInfo::SYMLINK;
    case boost::filesystem::file_not_found:
      return MetaInfo::DELETED;
    default:
      throw AbortIndex(
          "File type is unsuitable for indexing. Only Files, Directories and Symbolic links are supported");
  }
}

void ScanTask::update_fsattrib() {
  boost::filesystem::path babspath(abspath_.toStdWString());

  // First, preserve old values of attributes
  new_meta_.windowsAttrib(old_meta_.windowsAttrib());
  new_meta_.mode(old_meta_.mode());
  new_meta_.uid(old_meta_.uid());
  new_meta_.gid(old_meta_.gid());

  if (new_meta_.kind() != MetaInfo::SYMLINK)
    new_meta_.mtime(boost::filesystem::last_write_time(babspath));  // File/directory modification time
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
      new_meta_.mode(stat_buf.st_mode);
      new_meta_.uid(stat_buf.st_uid);
      new_meta_.gid(stat_buf.st_gid);
    }
  }
#endif
}

void ScanTask::update_chunks() {
  if (old_meta_.kind() == MetaInfo::FILE) {
    new_meta_.maxChunksize(old_meta_.maxChunksize());
    new_meta_.minChunksize(old_meta_.minChunksize());

    new_meta_.rabinMask(old_meta_.rabinMask());
    new_meta_.rabinShift(old_meta_.rabinShift());
    new_meta_.rabinPolynomial(old_meta_.rabinPolynomial());
  } else {
    new_meta_.maxChunksize(8 * 1024 * 1024);
    new_meta_.minChunksize(1 * 1024 * 1024);

    new_meta_.rabinMask(0x0FFFFF);
    new_meta_.rabinShift(53 - 8);
    new_meta_.rabinPolynomial(0x3DA3358B4DC173LL);

    // TODO: Generate a new polynomial for rabin_global_params here to prevent a possible fingerprinting attack.
  }

  // IV reuse
  QMap<QByteArray, QByteArray> pt_hmac__iv;
  for (auto& chunk : old_meta_.chunks()) {
    pt_hmac__iv.insert(chunk.ptKeyedHash(), chunk.iv());
  }

  // Initializing chunker
  Rabin rabin(new_meta_.rabinPolynomial(), new_meta_.rabinShift(), new_meta_.minChunksize(), new_meta_.maxChunksize(),
              new_meta_.rabinMask());

  // Chunking
  QList<ChunkInfo> chunks;

  QByteArray buffer;
  buffer.reserve(new_meta_.maxChunksize());

  QFile f(abspath_);
  if (!f.open(QIODevice::ReadOnly)) throw AbortIndex("I/O error: " + f.errorString());

  char byte;
  while (f.getChar(&byte) && !interrupted) {
    buffer.push_back(byte);

    if (rabin.next_chunk(byte)) {  // Found a chunk
      chunks.push_back(populate_chunk(buffer, pt_hmac__iv));
      buffer.clear();
    }
  }

  if (interrupted) throw AbortIndex("Indexing had been interruped");

  if (rabin.finalize()) chunks << populate_chunk(buffer, pt_hmac__iv);

  new_meta_.chunks(chunks);
}

ChunkInfo ScanTask::populate_chunk(const QByteArray& data, QMap<QByteArray, QByteArray> pt_hmac__iv) {
  qCDebug(log_indexer) << "New chunk size:" << data.size();
  ChunkInfo chunk;

  QCryptographicHash hasher(QCryptographicHash::Sha3_256);
  hasher.addData(secret_.encryptionKey());
  hasher.addData(data);

  chunk.ptKeyedHash(hasher.result());

  // IV reuse
  chunk.iv(pt_hmac__iv.value(chunk.ptKeyedHash(), generateRandomIV()));

  chunk.size(data.size());
  chunk.ctHash(ChunkInfo::compute_hash(ChunkInfo::encrypt(data, secret_.encryptionKey(), chunk.iv())));
  return chunk;
}

QByteArray ScanTask::pathKeyedHash() const {
  QByteArray normpath = PathNormalizer::normalizePath(abspath_, params_.path);
  return MetaInfo::makePathId(normpath, secret_);
}

} /* namespace librevault */
