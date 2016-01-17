/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include <src/util/parse_url.h>
#include "src/pch.h"
#include "src/util/Loggable.h"
#include <librevault/Secret.h>

namespace librevault {

class Config : protected Loggable {
	using milliseconds = std::chrono::milliseconds;
	using seconds = std::chrono::seconds;
public:
	Config(Loggable& parent_loggable, fs::path config_path);
	~Config();

	struct config_type {
		std::string device_name;
		url control_listen;
		url net_listen;
		bool net_natpmp_enabled;
		seconds net_natpmp_lifetime;
		seconds discovery_static_repeat_interval;
		bool discovery_multicast4_enabled;
		address_v4 discovery_multicast4_local_ip;
		address_v4 discovery_multicast4_ip;
		uint16_t discovery_multicast4_port;
		seconds discovery_multicast4_repeat_interval;
		bool discovery_multicast6_enabled;
		address_v6 discovery_multicast6_local_ip;
		address_v6 discovery_multicast6_ip;
		uint16_t discovery_multicast6_port;
		seconds discovery_multicast6_repeat_interval;
		bool discovery_bttracker_enabled;
		unsigned discovery_bttracker_num_want;
		seconds discovery_bttracker_min_interval;
		std::string discovery_bttracker_azureus_id;
		seconds discovery_bttracker_udp_reconnect_interval;
		seconds discovery_bttracker_udp_packet_timeout;
		std::vector<url> discovery_bttracker_trackers;

		struct FolderConfig {
			Secret secret;
			fs::path open_path;
			fs::path block_path;
			fs::path db_path;
			fs::path asm_path;
			std::vector<std::string> ignore_paths;
			std::vector<url> nodes;
			milliseconds index_event_timeout = milliseconds(1000);
			bool preserve_unix_attrib = false;
			bool preserve_windows_attrib = false;
			bool preserve_symlinks = true;
			cryptodiff::StrongHashType block_strong_hash_type = cryptodiff::StrongHashType::SHA3_224;
		};
		std::vector<FolderConfig> folders;
	} current;
	const config_type defaults;

	ptree get_ptree() const;
	void apply_ptree(const ptree& pt);

	using FolderConfig = config_type::FolderConfig;

private:
	fs::path config_path_;

	config_type convert_pt(const ptree& pt, const config_type& base) const;
	ptree convert_pt(const config_type& config) const;

	config_type make_default_config();
};

} /* namespace librevault */
