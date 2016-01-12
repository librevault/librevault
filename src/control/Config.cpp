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
#include "Config.h"

namespace librevault {

Config::Config(Loggable& parent_loggable, fs::path config_path) :
		Loggable(parent_loggable, "Config"),
		defaults(make_default_config()),
		config_path_(std::move(config_path)) {
	current = defaults;

	ptree file_pt;
	fs::ifstream options_fs(config_path_, std::ios::binary);
	if(options_fs){
		boost::property_tree::json_parser::read_json(options_fs, file_pt);
	}
	apply_ptree(file_pt);
}

Config::~Config() {
	auto file_pt = convert_pt(current);
	fs::ofstream options_fs(config_path_, std::ios::binary | std::ios::trunc);
	if(options_fs){
		boost::property_tree::json_parser::write_json(options_fs, file_pt);
	}
}

Config::config_type Config::make_default_config() {
	config_type config;
	config.device_name                                  = boost::asio::ip::host_name();

	config.control_listen                               = parse_url("127.0.0.1:61345");

	config.net_listen                                   = parse_url("[::]:51345");
	config.net_natpmp_enabled                           = true;
	config.net_natpmp_lifetime                          = seconds(3600);

	config.discovery_static_repeat_interval             = seconds(30);

	config.discovery_multicast4_enabled                 = true;
	config.discovery_multicast4_local_ip                = address_v4::from_string("0.0.0.0");
	config.discovery_multicast4_ip                      = address_v4::from_string("239.192.152.144");
	config.discovery_multicast4_port                    = 28914;
	config.discovery_multicast4_repeat_interval         = seconds(30);

	config.discovery_multicast6_enabled                 = true;
	config.discovery_multicast6_local_ip                = address_v6::from_string("::");
	config.discovery_multicast6_ip                      = address_v6::from_string("ff08::bd02");
	config.discovery_multicast6_port                    = 28914;
	config.discovery_multicast6_repeat_interval         = seconds(30);

	config.discovery_bttracker_enabled                  = true;
	config.discovery_bttracker_num_want                 = 30;
	config.discovery_bttracker_min_interval             = seconds(15);
	config.discovery_bttracker_azureus_id               = "-LV0001-";
	config.discovery_bttracker_udp_reconnect_interval   = seconds(30);
	config.discovery_bttracker_udp_packet_timeout       = seconds(10);

	config.discovery_bttracker_trackers                 .push_back(url("udp://tracker.openbittorrent.com:80"));
	config.discovery_bttracker_trackers                 .push_back(url("udp://open.demonii.com:1337"));
	config.discovery_bttracker_trackers                 .push_back(url("udp://tracker.coppersurfer.tk:6969"));
	config.discovery_bttracker_trackers                 .push_back(url("udp://tracker.leechers-paradise.org:6969"));

	config_type::FolderConfig folder_config;
	folder_config.index_event_timeout                   = milliseconds(1000);
	folder_config.preserve_unix_attrib                  = false;
	folder_config.preserve_windows_attrib               = false;
	folder_config.preserve_symlinks                     = true;
	folder_config.block_strong_hash_type                = cryptodiff::StrongHashType::SHA3_224;

	config.folders.push_back(folder_config);

	return config;
}

Config::config_type Config::convert_pt(const ptree& pt, const config_type& base) {
	config_type config;

	config.device_name                                  = pt.get("device_name", base.device_name);

	config.control_listen                               = parse_url(pt.get("control.listen", (std::string)base.control_listen));

	config.net_listen                                   = parse_url(pt.get("net.listen", (std::string)base.net_listen));
	config.net_natpmp_enabled                           = pt.get("net.natpmp.enabled", base.net_natpmp_enabled);
	config.net_natpmp_lifetime                          = seconds(pt.get("net.natpmp.lifetime", base.net_natpmp_lifetime.count()));

	config.discovery_static_repeat_interval             = seconds(pt.get("discovery.static.repeat_interval", base.discovery_static_repeat_interval.count()));

	config.discovery_multicast4_enabled                 = pt.get("discovery.multicast4.enabled", base.discovery_multicast4_enabled);
	config.discovery_multicast4_local_ip                = address_v4::from_string(pt.get("discovery.multicast4.local_ip", base.discovery_multicast4_local_ip.to_string()));
	config.discovery_multicast4_ip                      = address_v4::from_string(pt.get("discovery.multicast4.ip", base.discovery_multicast4_ip.to_string()));
	config.discovery_multicast4_port                    = pt.get("discovery.multicast4.port", base.discovery_multicast4_port);
	config.discovery_multicast4_repeat_interval         = seconds(pt.get("discovery.multicast4.repeat_interval", base.discovery_multicast4_repeat_interval.count()));

	config.discovery_multicast6_enabled                 = pt.get("discovery.multicast6.enabled", base.discovery_multicast6_enabled);
	config.discovery_multicast6_local_ip                = address_v6::from_string(pt.get("discovery.multicast6.local_ip", base.discovery_multicast6_local_ip.to_string()));
	config.discovery_multicast6_ip                      = address_v6::from_string(pt.get("discovery.multicast6.ip", base.discovery_multicast6_ip.to_string()));
	config.discovery_multicast6_port                    = pt.get("discovery.multicast6.port", base.discovery_multicast6_port);
	config.discovery_multicast6_repeat_interval         = seconds(pt.get("discovery.multicast6.repeat_interval", base.discovery_multicast6_repeat_interval.count()));

	config.discovery_bttracker_enabled                  = pt.get("discovery.bttracker.enabled", base.discovery_bttracker_enabled);
	config.discovery_bttracker_num_want                 = pt.get("discovery.bttracker.num_want", base.discovery_bttracker_num_want);
	config.discovery_bttracker_min_interval             = seconds(pt.get("discovery.bttracker.min_interval", base.discovery_bttracker_min_interval.count()));
	config.discovery_bttracker_azureus_id               = pt.get("discovery.bttracker.azureus_id", base.discovery_bttracker_azureus_id);
	config.discovery_bttracker_udp_reconnect_interval   = seconds(pt.get("discovery.bttracker.udp.reconnect_interval", base.discovery_bttracker_udp_reconnect_interval.count()));
	config.discovery_bttracker_udp_packet_timeout       = seconds(pt.get("discovery.bttracker.udp.packet_timeout", base.discovery_bttracker_udp_packet_timeout.count()));

	try {
		auto eqkey_it = pt.get_child("discovery.bttracker.trackers").equal_range("");
		for(auto it = eqkey_it.first; it != eqkey_it.second; it++) {
			config.discovery_bttracker_trackers.push_back(url(it->second.get<std::string>("")));
		}
	}catch(boost::property_tree::ptree_bad_path& e){
		config.discovery_bttracker_trackers = base.discovery_bttracker_trackers;
	}

	try {
		auto eqkey_it = pt.get_child("folders").equal_range("");
		for(auto it = eqkey_it.first; it != eqkey_it.second; it++) {
			const ptree& folder_pt = it->second;
			auto& default_folder = base.folders.front();

			config_type::FolderConfig folder_config;
			folder_config.secret                        = Secret(folder_pt.get<std::string>("secret"));
			folder_config.open_path                     = folder_pt.get<fs::path>("open_path");
			folder_config.block_path                    = folder_pt.get<fs::path>("block_path", "");
			folder_config.db_path                       = folder_pt.get<fs::path>("db_path", "");
			folder_config.asm_path                      = folder_pt.get<fs::path>("asm_path", "");

			folder_config.index_event_timeout           = milliseconds(folder_pt.get("index_event_timeout", default_folder.index_event_timeout.count()));
			folder_config.preserve_unix_attrib          = folder_pt.get("preserve_unix_attrib", default_folder.preserve_unix_attrib);
			folder_config.preserve_windows_attrib       = folder_pt.get("preserve_windows_attrib", default_folder.preserve_windows_attrib);
			folder_config.preserve_symlinks             = folder_pt.get("preserve_symlinks", default_folder.preserve_symlinks);
			folder_config.block_strong_hash_type        = cryptodiff::StrongHashType(folder_pt.get("block_strong_hash_type", (unsigned)default_folder.block_strong_hash_type));
			// node!
			config.folders.push_back(folder_config);
		}
	}catch(boost::property_tree::ptree_bad_path& e){}

	return config;
}

void Config::apply_ptree(const ptree& pt) {
	auto new_config = convert_pt(pt, current);
	std::swap(new_config, current);
}

ptree Config::convert_pt(const config_type& config) {
	ptree pt;
	pt.put("device_name", config.device_name);

	pt.put("control.listen", (std::string)config.control_listen);

	pt.put("net.listen", (std::string)config.net_listen);
	pt.put("net.netpmp.enabled", config.net_natpmp_enabled);
	pt.put("net.netpmp.lifetime", config.net_natpmp_lifetime.count());

	pt.put("discovery.static.repeat_interval", config.discovery_static_repeat_interval.count());

	pt.put("discovery.multicast4.enabled", config.discovery_multicast4_enabled);
	pt.put("discovery.multicast4.local_ip", config.discovery_multicast4_local_ip.to_string());
	pt.put("discovery.multicast4.ip", config.discovery_multicast4_ip.to_string());
	pt.put("discovery.multicast4.port", config.discovery_multicast4_port);
	pt.put("discovery.multicast4.repeat_interval", config.discovery_multicast4_repeat_interval.count());

	pt.put("discovery.multicast6.enabled", config.discovery_multicast6_enabled);
	pt.put("discovery.multicast6.local_ip", config.discovery_multicast6_local_ip.to_string());
	pt.put("discovery.multicast6.ip", config.discovery_multicast6_ip.to_string());
	pt.put("discovery.multicast6.port", config.discovery_multicast6_port);
	pt.put("discovery.multicast6.repeat_interval", config.discovery_multicast6_repeat_interval.count());

	pt.put("discovery.bttracker.enabled", config.discovery_bttracker_enabled);
	pt.put("discovery.bttracker.num_want", config.discovery_bttracker_num_want);
	pt.put("discovery.bttracker.min_interval", config.discovery_bttracker_min_interval.count());
	pt.put("discovery.bttracker.azureus_id", config.discovery_bttracker_azureus_id);
	pt.put("discovery.bttracker.udp.reconnect_interval", config.discovery_bttracker_udp_reconnect_interval.count());
	pt.put("discovery.bttracker.udp.packet_timeout", config.discovery_bttracker_udp_packet_timeout.count());

	ptree trackers_pt;
	for(auto& tracker : config.discovery_bttracker_trackers) {
		ptree tracker_pt;
		tracker_pt.put("", (std::string)tracker);
		trackers_pt.push_back({"", tracker_pt});
	}
	pt.add_child("discovery.bttracker.trackers", trackers_pt);

	if(!config.folders.empty()) {
		ptree folders_pt;
		for(auto& folder : config.folders) {
			ptree folder_pt;
			folder_pt.put("secret", folder.secret);
			folder_pt.put("open_path", folder.open_path);
			folder_pt.put("block_path", folder.block_path);
			folder_pt.put("db_path", folder.db_path);
			folder_pt.put("asm_path", folder.asm_path);

			folder_pt.put("index_event_timeout", folder.index_event_timeout.count());
			folder_pt.put("preserve_unix_attrib", folder.preserve_unix_attrib);
			folder_pt.put("preserve_windows_attrib", folder.preserve_windows_attrib);
			folder_pt.put("preserve_symlinks", folder.preserve_symlinks);
			folder_pt.put("block_strong_hash_type", folder.block_strong_hash_type);
			// Nodes for StaticDiscovery
			folders_pt.push_back({"", folder_pt});
		}
		pt.add_child("folders", folders_pt);
	}

	return pt;
}

} /* namespace librevault */
