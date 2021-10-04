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
#include "Archive.h"

#include <QTimer>
#include <boost/filesystem.hpp>

#include "NoArchive.h"
#include "TimestampArchive.h"
#include "TrashArchive.h"
#include "control/FolderParams.h"
#include "folder/PathNormalizer.h"
#include "folder/meta/MetaStorage.h"
#include "util/conv_fspath.h"

namespace librevault {

Archive::Archive(const FolderParams& params, MetaStorage* meta_storage, PathNormalizer* path_normalizer,
                 QObject* parent)
    : QObject(parent), meta_storage_(meta_storage), path_normalizer_(path_normalizer) {
  switch (params.archive_type) {
    case FolderParams::ArchiveType::NO_ARCHIVE:
      archive_strategy_ = new NoArchive(this);
      break;
    case FolderParams::ArchiveType::TIMESTAMP_ARCHIVE:
      archive_strategy_ = new TimestampArchive(params, path_normalizer_, this);
      break;
    case FolderParams::ArchiveType::TRASH_ARCHIVE:
      archive_strategy_ = new TrashArchive(params, path_normalizer_, this);
      break;
      //		case FolderParams::ArchiveType::BLOCK_ARCHIVE:
      //			archive_strategy_ = new NoArchive(this);
      //			break;
    default:
      throw std::runtime_error("Wrong Archive type");
  }
}

bool Archive::archive(const QString& denormpath) {
  boost::filesystem::path denormpath_fs = conv_fspath(denormpath);
  auto file_type = boost::filesystem::symlink_status(denormpath_fs).type();

  // Suppress unnecessary events on dir_monitor.
  meta_storage_->prepareAssemble(path_normalizer_->normalizePath(conv_fspath(denormpath_fs)), Meta::DELETED);

  if (file_type == boost::filesystem::directory_file) {
    if (boost::filesystem::is_empty(denormpath_fs))  // Okay, just remove this empty directory
      boost::filesystem::remove(denormpath_fs);
    else {  // Oh, damn, this is very NOT RIGHT! So, we have DELETED directory with NOT DELETED files in it
      for (auto it = boost::filesystem::directory_iterator(denormpath_fs);
           it != boost::filesystem::directory_iterator(); it++)
        archive(conv_fspath(it->path()));  // TODO: Okay, this is a horrible solution
      boost::filesystem::remove(denormpath_fs);
    }
  } else if (file_type == boost::filesystem::regular_file) {
    archive_strategy_->archive(denormpath);
  } else if (file_type == boost::filesystem::symlink_file || file_type == boost::filesystem::file_not_found) {
    boost::filesystem::remove(denormpath_fs);
  } else {
    qWarning() << "Unknown file type, nunable to archive:" << denormpath;
  }

  return true;  // FIXME: handle errors
}

}  // namespace librevault
