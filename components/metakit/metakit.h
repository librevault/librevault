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
#pragma once
#include "MetaInfo.h"
#include "secret/Secret.h"
#include <QString>
#include <chrono>
#include "util/exception.hpp"
#if defined(Q_OS_UNIX)
#  include <sys/stat.h>
#endif

namespace librevault {
namespace metakit {

class MetaInfoBuilder {
 public:
  DECLARE_EXCEPTION(ScanError, "Scan error");
  DECLARE_EXCEPTION_DETAIL(StatError, ScanError, "stat() call failed");
  DECLARE_EXCEPTION_DETAIL(MtimeError, ScanError, "Could not get modification time");
  DECLARE_EXCEPTION_DETAIL(UnsupportedKind, ScanError,
      "File type is unsuitable for indexing. Only Files, Directories and Symbolic links are "
      "supported");

  MetaInfoBuilder(QString path, QString root, Secret secret, bool preserve_symlinks);

  QByteArray makePathKeyedHash() const;

  qint64 getMtime();
  quint64 getMtimeGranularity();
  MetaInfo::Kind getKind();
  QByteArray getSymlinkTarget();

 private:
  QString path;
  QString root;
  Secret secret;
  bool preserve_symlinks;

#ifdef Q_OS_UNIX
  struct stat cached_stat;
  void updateStat();
#endif
};

int fuzzyCompareMtime(qint64 mtime1, quint64 gran1, qint64 mtime2, quint64 gran2);

}  // namespace metakit
}  // namespace librevault
