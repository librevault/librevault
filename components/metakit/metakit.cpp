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
#include "metakit.h"
#include "secret/Secret.h"
#include "path_normalizer.h"
#include "timestamp_granularity.h"
#include <MetaInfo.h>
#include <QCryptographicHash>
#include <QDir>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace librevault {
namespace metakit {

MetaInfoBuilder::MetaInfoBuilder(QString path, QString root, Secret secret, bool preserve_symlinks)
    : path(std::move(path)),
      root(std::move(root)),
      secret(secret),
      preserve_symlinks(preserve_symlinks) {}

QByteArray MetaInfoBuilder::makePathKeyedHash() const {
  QCryptographicHash hasher(QCryptographicHash::Sha3_256);
  hasher.addData(secret.encryptionKey());
  hasher.addData(metakit::normalizePath(path, root));
  return hasher.result();
}

qint64 MetaInfoBuilder::getMtime() {
  std::chrono::nanoseconds result{};

#if defined(Q_OS_LINUX)
  updateStat();
  result = std::chrono::nanoseconds(
      cached_stat.st_mtim.tv_sec * 1000000000ll + cached_stat.st_mtim.tv_nsec);
#else
  result = std::chrono::seconds(fs::last_write_time(path.toStdWString()));
#endif

  return result.count();
}

quint64 MetaInfoBuilder::getMtimeGranularity() {
  return (quint64)makeTimestampGranularity(path).count();
}

MetaInfo::Kind MetaInfoBuilder::getKind() {
  fs::file_status file_status =
      preserve_symlinks ? fs::symlink_status(path.toStdWString()) : fs::status(path.toStdWString());

  switch (file_status.type()) {
    case fs::regular_file: return MetaInfo::FILE;
    case fs::directory_file: return MetaInfo::DIRECTORY;
    case fs::symlink_file: return MetaInfo::SYMLINK;
    case fs::file_not_found: return MetaInfo::DELETED;
    default: throw UnsupportedKind();
  }
}

QByteArray MetaInfoBuilder::getSymlinkTarget() {
  return QByteArray::fromStdString(
      boost::filesystem::read_symlink(path.toStdWString()).generic_string());
}

#ifdef Q_OS_UNIX
void MetaInfoBuilder::updateStat() {
  int stat_err =
      preserve_symlinks ? lstat(path.toUtf8(), &cached_stat) : stat(path.toUtf8(), &cached_stat);
  if (stat_err) throw StatError();
}
#endif

int fuzzyCompareMtime(qint64 mtime1, quint64 gran1, qint64 mtime2, quint64 gran2) {
  quint64 granula_common = std::max(std::max(gran1, gran2), 1ull);
  mtime1 -= (mtime1 % granula_common);
  mtime2 -= (mtime2 % granula_common);
  if (mtime1 < mtime2)
    return -1;
  else if (mtime1 > mtime2)
    return 1;
  else
    return 0;
}

}  // namespace metakit
}  // namespace librevault
