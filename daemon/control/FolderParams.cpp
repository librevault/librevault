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

namespace librevault {

FolderParams::FolderParams(QJsonObject json_params) {
	// Necessary
	secret = json_params["secret"].toString().toStdString();
	path = json_params["path"].toString();

	// Optional
	system_path = json_params["system_path"].toString(path + "/.librevault");
	index_event_timeout = std::chrono::milliseconds(json_params["index_event_timeout"].toInt());
	preserve_unix_attrib = json_params["preserve_unix_attrib"].toBool();
	preserve_windows_attrib = json_params["preserve_windows_attrib"].toBool();
	preserve_symlinks = json_params["preserve_symlinks"].toBool();
	normalize_unicode = json_params["normalize_unicode"].toBool();
	chunk_strong_hash_type = Meta::StrongHashType(json_params["chunk_strong_hash_type"].toInt());
	full_rescan_interval = std::chrono::seconds(json_params["full_rescan_interval"].toInt());

	foreach(const QJsonValue& ignore_path, json_params["ignore_paths"].toArray())
		ignore_paths.push_back(ignore_path.toString());
	foreach(const QJsonValue& node, json_params["nodes"].toArray())
		nodes.push_back(node.toString());

	QString archive_type_str = json_params["archive_type"].toString();
	if(archive_type_str == "none")
		archive_type = ArchiveType::NO_ARCHIVE;
	if(archive_type_str == "trash")
		archive_type = ArchiveType::TRASH_ARCHIVE;
	if(archive_type_str == "timestamp")
		archive_type = ArchiveType::TIMESTAMP_ARCHIVE;
	if(archive_type_str == "block")
		archive_type = ArchiveType::BLOCK_ARCHIVE;

	archive_trash_ttl = json_params["archive_trash_ttl"].toInt();
	archive_timestamp_count = json_params["archive_timestamp_count"].toInt();
	mainline_dht_enabled = json_params["mainline_dht_enabled"].toBool();
}

} /* namespace librevault */
