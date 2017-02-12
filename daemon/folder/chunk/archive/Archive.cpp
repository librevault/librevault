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
#include "Archive.h"
#include "NoArchive.h"
#include "TimestampArchive.h"
#include "TrashArchive.h"
#include "control/FolderParams.h"
#include "folder/PathNormalizer.h"
#include "folder/meta/MetaStorage.h"
#include <QTimer>
#include <boost/filesystem.hpp>
#include <regex>

namespace librevault {

Archive::Archive(const FolderParams& params, MetaStorage* meta_storage, PathNormalizer* path_normalizer, QObject* parent) :
	QObject(parent),
	meta_storage_(meta_storage),
	path_normalizer_(path_normalizer) {

	switch(params.archive_type) {
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
		default: throw std::runtime_error("Wrong Archive type");
	}
}

void Archive::archive(const fs::path& from) {
	auto file_type = fs::symlink_status(from).type();

	// Suppress unnecessary events on dir_monitor.
	meta_storage_->prepareAssemble(path_normalizer_->normalizePath(QString::fromStdWString(from.wstring())), Meta::DELETED);

	if(file_type == fs::directory_file) {
		if(fs::is_empty(from)) // Okay, just remove this empty directory
			fs::remove(from);
		else {  // Oh, damn, this is very NOT RIGHT! So, we have DELETED directory with NOT DELETED files in it
			for(auto it = fs::directory_iterator(from); it != fs::directory_iterator(); it++)
				archive(it->path()); // TODO: Okay, this is a horrible solution
			fs::remove(from);
		}
	}else if(file_type == fs::regular_file) {
		archive_strategy_->archive(from);
	}else if(file_type == fs::symlink_file || file_type == fs::file_not_found) {
		fs::remove(from);
	}
	// TODO: else
}

bool Archive::archive(QString denormpath) {
	archive(denormpath.toStdWString());
	return true;    // FIXME: handle errors
}

} /* namespace librevault */
