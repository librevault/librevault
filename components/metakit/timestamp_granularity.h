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
#include <QStorageInfo>
#include <chrono>

namespace librevault {
namespace metakit {

std::chrono::nanoseconds makeTimestampGranularity(const QString& path) {
  using namespace std::chrono_literals;

  QString filesystem = QString::fromUtf8(QStorageInfo(path).fileSystemType());
  // Windows
  if (filesystem.contains("ntfs", Qt::CaseInsensitive)) return 100ns;
  if (filesystem.contains("exfat", Qt::CaseInsensitive)) return 10ms;
  if (filesystem.contains("fat", Qt::CaseInsensitive)) return 2s;
  // Mac
  if (filesystem.contains("hfs", Qt::CaseInsensitive)) return 1s;
  // Linux
  if (filesystem.contains("ext2", Qt::CaseInsensitive)) return 1s;
  if (filesystem.contains("ext3", Qt::CaseInsensitive)) return 1s;
  if (filesystem.contains("reiserfs", Qt::CaseInsensitive)) return 1s;
  // Other
  if (filesystem.contains("udf", Qt::CaseInsensitive)) return 1ms;
  // ext4, btrfs, f2fs, xfs, zfs, reiser4, apfs are considered as supporting nanosecond resolution
  return 1ns;
}

}  // namespace metakit
}  // namespace librevault
