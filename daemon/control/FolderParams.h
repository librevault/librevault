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
#include "util/parse_url.h"
#include <json/json.h>
#include <librevault/Meta.h>
#include <librevault/Secret.h>
#include <boost/filesystem/path.hpp>
#include <chrono>

namespace librevault {

struct FolderParams {
	enum class ArchiveType : unsigned {
		NO_ARCHIVE = 0,
		TRASH_ARCHIVE,
		TIMESTAMP_ARCHIVE,
		BLOCK_ARCHIVE
	};

	FolderParams(){}
	FolderParams(const Json::Value& json_params) {
		FolderParams defaults;

		// Necessary
		secret = json_params["secret"].asString();
		path = json_params["path"].asString();

		// Optional
		system_path = json_params.get("system_path", (path / ".librevault").string()).asString();
		index_event_timeout = std::chrono::milliseconds(json_params.get("index_event_timeout", Json::Value::UInt64(defaults.index_event_timeout.count())).asUInt64());
		preserve_unix_attrib = json_params.get("preserve_unix_attrib", defaults.preserve_unix_attrib).asBool();
		preserve_windows_attrib = json_params.get("preserve_windows_attrib", defaults.preserve_windows_attrib).asBool();
		preserve_symlinks = json_params.get("preserve_symlinks", defaults.preserve_symlinks).asBool();
		normalize_unicode = json_params.get("normalize_unicode", defaults.normalize_unicode).asBool();
		chunk_strong_hash_type = Meta::StrongHashType(json_params.get("chunk_strong_hash_type", defaults.chunk_strong_hash_type).asUInt());
		full_rescan_interval = std::chrono::seconds(json_params.get("full_rescan_interval", Json::Value::UInt64(defaults.full_rescan_interval.count())).asUInt64());

		for(auto ignore_path : json_params["ignore_paths"])
			ignore_paths.push_back(ignore_path.asString());
		for(auto node : json_params["nodes"])
			nodes.push_back(node.asString());

		auto archive_type_str = json_params.get("archive_type", "trash").asString();
		if(archive_type_str == "none")
			archive_type = ArchiveType::NO_ARCHIVE;
		if(archive_type_str == "trash")
			archive_type = ArchiveType::TRASH_ARCHIVE;
		if(archive_type_str == "timestamp")
			archive_type = ArchiveType::TIMESTAMP_ARCHIVE;
		if(archive_type_str == "block")
			archive_type = ArchiveType::BLOCK_ARCHIVE;

		archive_trash_ttl = json_params.get("archive_trash_ttl", defaults.archive_trash_ttl).asUInt();
		archive_timestamp_count = json_params.get("archive_timestamp_count", defaults.archive_timestamp_count).asUInt();
		mainline_dht_enabled = json_params.get("mainline_dht_enabled", defaults.mainline_dht_enabled).asBool();
	}

	/* Parameters */
	Secret secret;
	boost::filesystem::path path;
	boost::filesystem::path system_path;
	std::chrono::milliseconds index_event_timeout = std::chrono::milliseconds(1000);
	bool preserve_unix_attrib = false;
	bool preserve_windows_attrib = false;
	bool preserve_symlinks = true;
	bool normalize_unicode = true;
	Meta::StrongHashType chunk_strong_hash_type = Meta::StrongHashType::SHA3_224;
	std::chrono::seconds full_rescan_interval = std::chrono::seconds(600);
	std::vector<std::string> ignore_paths;
	std::vector<url> nodes;
	ArchiveType archive_type = ArchiveType::TRASH_ARCHIVE;
	unsigned archive_trash_ttl = 30;
	unsigned archive_timestamp_count = 5;
	bool mainline_dht_enabled = true;
};

} /* namespace librevault */
