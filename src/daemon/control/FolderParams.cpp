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
#include "FolderParams.h"
#include <QJsonArray>
#include <QtDebug>

namespace librevault {

FolderParams::FolderParams(QVariantMap fconfig) {
	// Necessary
	secret = fconfig["secret"].toString().toStdString();
	path = fconfig["path"].toString();

	// Optional
	system_path = fconfig["system_path"].isValid() ? fconfig["system_path"].toString() : path + "/.librevault";
	index_event_timeout = std::chrono::milliseconds(fconfig["index_event_timeout"].toInt());
	preserve_unix_attrib = fconfig["preserve_unix_attrib"].toBool();
	preserve_windows_attrib = fconfig["preserve_windows_attrib"].toBool();
	preserve_symlinks = fconfig["preserve_symlinks"].toBool();
	normalize_unicode = fconfig["normalize_unicode"].toBool();
	chunk_strong_hash_type = Meta::StrongHashType(fconfig["chunk_strong_hash_type"].toInt());
	full_rescan_interval = std::chrono::seconds(fconfig["full_rescan_interval"].toInt());

	for(const QString& ignore_path : fconfig["ignore_paths"].toStringList())
		ignore_paths.push_back(ignore_path);
	for(const QString& node : fconfig["nodes"].toStringList())
		nodes.push_back(node);

	QString archive_type_str = fconfig["archive_type"].toString();
	if(archive_type_str == "none")
		archive_type = ArchiveType::NO_ARCHIVE;
	if(archive_type_str == "trash")
		archive_type = ArchiveType::TRASH_ARCHIVE;
	if(archive_type_str == "timestamp")
		archive_type = ArchiveType::TIMESTAMP_ARCHIVE;
	if(archive_type_str == "block")
		archive_type = ArchiveType::BLOCK_ARCHIVE;

	archive_trash_ttl = fconfig["archive_trash_ttl"].toInt();
	archive_timestamp_count = fconfig["archive_timestamp_count"].toInt();
	mainline_dht_enabled = fconfig["mainline_dht_enabled"].toBool();
}

} /* namespace librevault */
