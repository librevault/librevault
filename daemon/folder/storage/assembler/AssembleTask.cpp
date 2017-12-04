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
#include "AssembleTask.h"

#include "config/models.h"
#include "folder/IgnoreList.h"
#include "folder/storage/ChunkStorage.h"
#include "folder/storage/Index.h"
#include "util/conv_fspath.h"
#include <ChunkInfo.h>
#include <path_normalizer.h>
#include <QBitArray>
#include <QDir>
#include <QLoggingCategory>
#include <QSaveFile>
#include <boost/filesystem.hpp>
#if defined(Q_OS_UNIX)
#  include <sys/stat.h>
#elif defined(Q_OS_WIN)
#  include <windows.h>
#endif

Q_LOGGING_CATEGORY(log_assembler, "folder.storage.assembler")

namespace librevault {

AssembleTask::AssembleTask(SignedMeta smeta, const FolderSettings& params, Index* index,
    ChunkStorage* chunk_storage, QObject* parent)
    : QueuedTask(ASSEMBLE, parent),
      params_(params),
      index_(index),
      chunk_storage_(chunk_storage),
      smeta_(std::move(smeta)),
      meta_(smeta_.metaInfo()) {}

AssembleTask::~AssembleTask() = default;

QByteArray AssembleTask::getChunkPt(const QByteArray& ct_hash) const {
  try {
    QPair<quint32, QByteArray> size_iv = index_->getChunkSizeIv(ct_hash);
    return ChunkInfo::decrypt(chunk_storage_->getChunk(ct_hash), params_.secret, size_iv.second);
  } catch (std::exception& e) {
    qCWarning(log_assembler)
        << "Could not get plaintext chunk (which is marked as existing in index), DB collision";
    throw ChunkStorage::NoSuchChunk();
  }
}

void AssembleTask::run() noexcept {
  normpath_ = meta_.path().plaintext(params_.secret.encryptionKey());
  denormpath_ = metakit::absolutizePath(normpath_, params_.path);

  try {
    bool assembled = false;
    switch (meta_.kind()) {
      case MetaInfo::FILE: assembled = assembleFile(); break;
      case MetaInfo::DIRECTORY: assembled = assembleDirectory(); break;
      case MetaInfo::SYMLINK: assembled = assembleSymlink(); break;
      case MetaInfo::DELETED: assembled = assembleDeleted(); break;
      default:
        qWarning() << QString("Unexpected meta type: %1").arg(meta_.kind());
        throw abortAssemble();
    }
    if (assembled) {
      if (meta_.kind() != MetaInfo::DELETED) applyAttrib();

      index_->setAssembled(meta_.pathKeyedHash());
      for (const auto& chunk : meta_.chunks()) chunk_storage_->gcChunk(chunk.ctHash());
    }
  } catch (abortAssemble& e) {  // Already handled
  } catch (std::exception& e) {
    qCWarning(log_assembler) << "Unknown exception while assembling:"
                             << meta_.path().plaintext(params_.secret.encryptionKey())
                             << "E:" << e.what();  // FIXME: #83
  }
}

bool AssembleTask::assembleDeleted() {
  qDebug(log_assembler) << "Assembling DELETED entry";
  return QFile::remove(denormpath_);
}

bool AssembleTask::assembleSymlink() {
  qDebug(log_assembler) << "Assembling SYMLINK entry";

  boost::filesystem::path denormpath_fs(denormpath_.toStdWString());

  boost::filesystem::remove_all(denormpath_fs);
  boost::filesystem::create_symlink(
      meta_.symlinkTarget().plaintext(params_.secret.encryptionKey()).toStdString(), denormpath_fs);

  return true;  // Maybe, something else?
}

bool AssembleTask::assembleDirectory() {
  qDebug(log_assembler) << "Assembling DIRECTORY entry";

  boost::filesystem::path denormpath_fs(denormpath_.toStdWString());

  bool create_new = true;
  if (boost::filesystem::status(denormpath_fs).type() !=
      boost::filesystem::file_type::directory_file)
    create_new = !boost::filesystem::remove(denormpath_fs);

  if (create_new) QDir().mkpath(denormpath_);

  return true;  // Maybe, something else?
}

bool AssembleTask::assembleFile() {
  qDebug(log_assembler) << "Assembling FILE entry";

  // Check if we have all needed chunks
  auto bitfield = chunk_storage_->makeBitfield(meta_);
  if (bitfield.count(true) != bitfield.size()) return false;

  //
  QString assembly_path =
      params_.effectiveSystemPath() + "/" +
      conv_fspath(boost::filesystem::unique_path("assemble-%%%%-%%%%-%%%%-%%%%"));

  // TODO: Check for assembled chunk and try to extract them and push into encstorage.
  QSaveFile assembly_f(assembly_path);  // Opening file
  if (!assembly_f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qCWarning(log_assembler) << "File cannot be opened:" << assembly_path
                             << "E:" << assembly_f.errorString();  // FIXME: #83
    throw abortAssemble();
  }

  for (auto chunk : meta_.chunks()) {
    assembly_f.write(getChunkPt(chunk.ctHash()));  // Writing to file
  }

  if (!assembly_f.commit()) {
    qCWarning(log_assembler) << "File cannot be written:" << assembly_path
                             << "E:" << assembly_f.errorString();  // FIXME: #83
    throw abortAssemble();
  }

  {
    boost::system::error_code ec;
    boost::filesystem::last_write_time(conv_fspath(assembly_path), meta_.mtime(), ec);
    if (ec) {
      qCWarning(log_assembler) << "Could not set mtime on file:" << assembly_path
                               << "E:" << QString::fromStdString(ec.message());  // FIXME: #83
    }
  }

  if (QFile::exists(denormpath_) && !QFile::remove(denormpath_)) {
    qCWarning(log_assembler) << "Item cannot be archived/removed:" << denormpath_;  // FIXME: #83
    throw abortAssemble();
  }
  if (!QFile::rename(assembly_path, denormpath_)) {
    qCWarning(log_assembler) << "File cannot be moved to its final location:" << denormpath_
                             << "Current location:" << assembly_path;  // FIXME: #83
    throw abortAssemble();
  }

  return true;
}

void AssembleTask::applyAttrib() {
#if defined(Q_OS_UNIX)
  if (params_.preserve_unix_attrib) {
    if (meta_.kind() != MetaInfo::SYMLINK) {
      int ec = 0;
      ec = chmod(QFile::encodeName(denormpath_), meta_.mode());
      if (ec) qCWarning(log_assembler) << "Error applying mode to" << denormpath_;  // FIXME: #83

      ec = chown(QFile::encodeName(denormpath_), meta_.uid(), meta_.gid());
      if (ec) qCWarning(log_assembler) << "Error applying uid/gid to" << denormpath_;
    }
  }
#elif defined(Q_OS_WIN)
  // Apply Windows attrib here.
#endif
}

QByteArray AssembleTask::pathKeyedHash() const { return smeta_.metaInfo().pathKeyedHash(); }

} /* namespace librevault */
