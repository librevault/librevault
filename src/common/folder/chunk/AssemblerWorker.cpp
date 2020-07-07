#include "AssemblerWorker.h"

#include <QDir>
#include <QLoggingCategory>
#include <QSaveFile>
#include <boost/filesystem.hpp>

#include "ChunkStorage.h"
#include "control/FolderParams.h"
#include "folder/IgnoreList.h"
#include "folder/PathNormalizer.h"
#include "folder/chunk/archive/Archive.h"
#include "folder/meta/MetaStorage.h"
#include "util/conv_fspath.h"
#ifdef Q_OS_UNIX
#include <sys/stat.h>
#endif
#ifdef Q_OS_WIN
#include <windows.h>
#endif

Q_LOGGING_CATEGORY(log_assembler, "folder.chunk.assembler")

namespace librevault {

AssemblerWorker::AssemblerWorker(const SignedMeta& smeta, const FolderParams& params, MetaStorage* meta_storage,
                                 ChunkStorage* chunk_storage, PathNormalizer* path_normalizer, Archive* archive)
    : params_(params),
      meta_storage_(meta_storage),
      chunk_storage_(chunk_storage),
      path_normalizer_(path_normalizer),
      archive_(archive),
      meta_(smeta.meta()) {}

AssemblerWorker::~AssemblerWorker() {}

QByteArray AssemblerWorker::get_chunk_pt(const QByteArray& ct_hash) const {
  auto chunk = chunk_storage_->get_chunk(ct_hash);

  try {
    QPair<quint32, QByteArray> size_iv = meta_storage_->getChunkSizeIv(ct_hash);
    return Meta::Chunk::decrypt(chunk, size_iv.first, params_.secret.get_Encryption_Key(), size_iv.second);
  } catch (std::exception& e) {
    qCWarning(log_assembler) << "Could not get plaintext chunk (which is marked as existing in index), DB collision";
    throw ChunkStorage::ChunkNotFound();
  }
}

void AssemblerWorker::run() noexcept {
  LOGFUNC();

  normpath_ = meta_.path(params_.secret);
  denormpath_ = path_normalizer_->denormalizePath(normpath_);

  try {
    bool assembled = false;
    switch (meta_.meta_type()) {
      case Meta::FILE:
        assembled = assemble_file();
        break;
      case Meta::DIRECTORY:
        assembled = assemble_directory();
        break;
      case Meta::SYMLINK:
        assembled = assemble_symlink();
        break;
      case Meta::DELETED:
        assembled = assemble_deleted();
        break;
      default:
        qWarning() << QString("Unexpected meta type: %1").arg(meta_.meta_type());
        throw abort_assembly();
    }
    if (assembled) {
      if (meta_.meta_type() != Meta::DELETED) apply_attrib();

      meta_storage_->markAssembled(meta_.path_id());
      chunk_storage_->cleanup(meta_);
    }
  } catch (abort_assembly& e) {  // Already handled
  } catch (std::exception& e) {
    qCWarning(log_assembler) << "Unknown exception while assembling:" << meta_.path(params_.secret)
                             << "E:" << e.what();  // FIXME: #83
  }
}

bool AssemblerWorker::assemble_deleted() {
  LOGFUNC();

  return archive_->archive(denormpath_);  // Maybe, something else?
}

bool AssemblerWorker::assemble_symlink() {
  LOGFUNC();

  boost::filesystem::path denormpath_fs(denormpath_.toStdWString());

  boost::filesystem::remove_all(denormpath_fs);
  boost::filesystem::create_symlink(conv_fspath(meta_.symlink_path(params_.secret)), denormpath_fs);

  return true;  // Maybe, something else?
}

bool AssemblerWorker::assemble_directory() {
  LOGFUNC();

  boost::filesystem::path denormpath_fs(denormpath_.toStdWString());

  bool create_new = true;
  if (boost::filesystem::status(denormpath_fs).type() != boost::filesystem::file_type::directory_file)
    create_new = !boost::filesystem::remove(denormpath_fs);
  meta_storage_->prepareAssemble(normpath_, Meta::DIRECTORY, create_new);

  if (create_new) QDir().mkpath(denormpath_);

  return true;  // Maybe, something else?
}

bool AssemblerWorker::assemble_file() {
  LOGFUNC();

  // Check if we have all needed chunks
  if (chunk_storage_->make_bitfield(meta_) != QBitArray(meta_.chunks().size(), true)) return false;  // retreat!

  QString assembly_path =
      params_.system_path + "/" + conv_fspath(boost::filesystem::unique_path("assemble-%%%%-%%%%-%%%%-%%%%"));

  // TODO: Check for assembled chunk and try to extract them and push into encstorage.
  QSaveFile assembly_f(assembly_path);  // Opening file
  if (!assembly_f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qCWarning(log_assembler) << "File cannot be opened:" << assembly_path
                             << "E:" << assembly_f.errorString();  // FIXME: #83
    throw abort_assembly();
  }

  for (auto chunk : meta_.chunks()) {
    assembly_f.write(get_chunk_pt(chunk.ct_hash));  // Writing to file
  }

  if (!assembly_f.commit()) {
    qCWarning(log_assembler) << "File cannot be written:" << assembly_path
                             << "E:" << assembly_f.errorString();  // FIXME: #83
    throw abort_assembly();
  }

  {
    boost::system::error_code ec;
    boost::filesystem::last_write_time(conv_fspath(assembly_path), meta_.mtime(), ec);
    if (ec) {
      qCWarning(log_assembler) << "Could not set mtime on file:" << assembly_path
                               << "E:" << QString::fromStdString(ec.message());  // FIXME: #83
    }
  }

  meta_storage_->prepareAssemble(normpath_, Meta::FILE, boost::filesystem::exists(conv_fspath(denormpath_)));

  if (!archive_->archive(denormpath_)) {
    qCWarning(log_assembler) << "Item cannot be archived/removed:" << denormpath_;  // FIXME: #83
    throw abort_assembly();
  }
  if (!QFile::rename(assembly_path, denormpath_)) {
    qCWarning(log_assembler) << "File cannot be moved to its final location:" << denormpath_
                             << "Current location:" << assembly_path;  // FIXME: #83
    throw abort_assembly();
  }

  return true;
}

void AssemblerWorker::apply_attrib() {
#if defined(Q_OS_UNIX)
  if (params_.preserve_unix_attrib) {
    if (meta_.meta_type() != Meta::SYMLINK) {
      int ec = 0;
      ec = chmod(QFile::encodeName(denormpath_), meta_.mode());
      if (ec) {
        qCWarning(log_assembler) << "Error applying mode to" << denormpath_;  // FIXME: #83
      }

      ec = chown(QFile::encodeName(denormpath_), meta_.uid(), meta_.gid());
      if (ec) {
        qCWarning(log_assembler) << "Error applying uid/gid to" << denormpath_;
      }
    }
  }
#elif defined(Q_OS_WIN)
  // Apply Windows attrib here.
#endif
}

}  // namespace librevault
