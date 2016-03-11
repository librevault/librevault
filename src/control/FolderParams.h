/* Copyright (C) 2016 Alexander Shishenko <GamePad64@gmail.com>
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
#pragma once
#include "src/pch.h"
#include "src/util/parse_url.h"
#include <json/json.h>
#include <librevault/Meta.h>
#include <librevault/Secret.h>

namespace librevault {

struct FolderParams {
	// The constructor accepts only *merged* json values, without null values
	FolderParams(const Json::Value& json_params) :
		secret(json_params["secret"].asString()),
		path(json_params["path"].asString()),
		system_path(json_params["system_path"].asString()),
		index_event_timeout(json_params["index_event_timeout"].asUInt64()),
		preserve_unix_attrib(json_params["preserve_unix_attrib"].asBool()),
		preserve_windows_attrib(json_params["preserve_windows_attrib"].asBool()),
		preserve_symlinks(json_params["preserve_symlinks"].asBool()),
		chunk_strong_hash_type(Meta::StrongHashType(json_params["chunk_strong_hash_type"].asInt())) {

		if(system_path.empty()) system_path = path / ".librevault";

		for(auto ignore_path : json_params["ignore_paths"])
			ignore_paths.push_back(ignore_path.asString());
		for(auto node : json_params["nodes"])
			nodes.push_back(node.asString());
	}

	/* Parameters */
	Secret secret;
	fs::path path;
	fs::path system_path;
	std::chrono::milliseconds index_event_timeout = std::chrono::milliseconds(1000);
	bool preserve_unix_attrib = false;
	bool preserve_windows_attrib = false;
	bool preserve_symlinks = true;
	Meta::StrongHashType chunk_strong_hash_type = Meta::StrongHashType::SHA3_224;
	std::vector<std::string> ignore_paths;
	std::vector<url> nodes;
};

} /* namespace librevault */
