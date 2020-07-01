#include "IndexerWorker.h"

#include <rabin.h>

#include <QFile>
#include <QtConcurrent/QtConcurrentRun>
#include <boost/filesystem.hpp>

#include "MetaStorage.h"
#include "control/FolderParams.h"
#include "crypto/AES_CBC.h"
#include "crypto/KMAC-SHA3.h"
#include "folder/IgnoreList.h"
#include "folder/PathNormalizer.h"
#include "util/conv_fspath.h"
#include "util/human_size.h"

#ifdef Q_OS_UNIX
#include <sys/stat.h>
#endif
#ifdef Q_OS_WIN
#include <windows.h>
#endif

Q_DECLARE_LOGGING_CATEGORY(log_indexer)

namespace librevault {

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

    try {
      old_smeta_ = meta_storage_->getMeta(Meta::make_path_id(normpath, secret_));
      old_meta_ = old_smeta_.meta();
      if (boost::filesystem::last_write_time(abspath_.toStdString()) == old_meta_.mtime())
        throw AbortIndex("Modification time is not changed");
    } catch (boost::filesystem::filesystem_error& e) {
    } catch (MetaStorage::MetaNotFound& e) {
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
  boost::filesystem::path babspath = conv_fspath(abspath_);

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
  Meta::RabinGlobalParams rabin_global_params;

  if (old_meta_.meta_type() == Meta::FILE && old_meta_.validate()) {
    new_meta_.set_algorithm_type(old_meta_.algorithm_type());
    new_meta_.set_strong_hash_type(old_meta_.strong_hash_type());

    new_meta_.set_max_chunksize(old_meta_.max_chunksize());
    new_meta_.set_min_chunksize(old_meta_.min_chunksize());

    new_meta_.raw_rabin_global_params() = old_meta_.raw_rabin_global_params();

    rabin_global_params = old_meta_.rabin_global_params(secret_);
  } else {
    new_meta_.set_algorithm_type(Meta::RABIN);
    new_meta_.set_strong_hash_type(params_.chunk_strong_hash_type);

    new_meta_.set_max_chunksize(8 * 1024 * 1024);
    new_meta_.set_min_chunksize(1 * 1024 * 1024);

    // TODO: Generate a new polynomial for rabin_global_params here to prevent a possible fingerprinting attack.
  }

  // IV reuse
  QHash<QByteArray, QByteArray> pt_hmac__iv;
  for (auto& chunk : old_meta_.chunks()) pt_hmac__iv.insert(chunk.pt_hmac, chunk.iv);

  // Initializing chunker
  rabin_t hasher;
  hasher.average_bits = rabin_global_params.avg_bits;
  hasher.minsize = new_meta_.min_chunksize();
  hasher.maxsize = new_meta_.max_chunksize();
  hasher.polynomial = rabin_global_params.polynomial;
  hasher.polynomial_degree = rabin_global_params.polynomial_degree;
  hasher.polynomial_shift = rabin_global_params.polynomial_shift;

  hasher.mask = uint64_t((1 << uint64_t(hasher.average_bits)) - 1);

  rabin_init(&hasher);

  // Chunking
  QFile f(abspath_);
  if (!f.open(QIODevice::ReadOnly)) throw AbortIndex("I/O error: " + f.errorString());

  QVector<Meta::Chunk> chunks;
  chunks.reserve(f.size() / hasher.minsize);

  QByteArray data;
  data.reserve(hasher.maxsize);

  for (char c; f.getChar(&c) && active_;) {
    data += c;
    if (rabin_next_chunk(&hasher, (uint8_t*)&c, 1) == 1) {
      // Found a chunk
      chunks += populate_chunk(data, pt_hmac__iv);
      data.truncate(0);
    }
  }

  if (!active_) throw AbortIndex("Indexing had been interruped");
  if (rabin_finalize(&hasher) != 0) chunks += populate_chunk(data, pt_hmac__iv);

  chunks.shrink_to_fit();

  new_meta_.set_chunks(chunks);
}

Meta::Chunk IndexerWorker::populate_chunk(const QByteArray& data, const QHash<QByteArray, QByteArray>& pt_hmac__iv) {
  qCDebug(log_indexer) << "New chunk size:" << data.size();
  Meta::Chunk chunk;

  chunk.pt_hmac = data | crypto::KMAC_SHA3_224(secret_.get_Encryption_Key());

  // IV reuse
  chunk.iv = pt_hmac__iv.value(chunk.pt_hmac, crypto::AES_CBC::randomIv());

  chunk.size = data.size();
  chunk.ct_hash = Meta::Chunk::computeStrongHash(Meta::Chunk::encrypt(data, secret_.get_Encryption_Key(), chunk.iv),
                                                 new_meta_.strong_hash_type());
  return chunk;
}

}  // namespace librevault
