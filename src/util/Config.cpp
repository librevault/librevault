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

Config::Config(Loggable& parent_loggable, fs::path config_path) : Loggable(parent_loggable, "Config"), config_path_(std::move(config_path)) {
	persistent_config();
	log_->info() << "Configuration file loaded: " << config_path_;

	boost::property_tree::json_parser::write_json(std::cout, *file_config_ptree_);
	boost::property_tree::json_parser::write_json(std::cout, merged_config());
}

Config::~Config() {
	fs::ofstream options_fs(config_path_, std::ios::trunc | std::ios::binary);
	boost::property_tree::json_parser::write_json(options_fs, *file_config_ptree_);
}

const ptree& Config::general_config() const {
	std::lock_guard<decltype(merged_config_mtx_)> default_config_mtx_lk(merged_config_mtx_);
	if(merged_config_ptree_) return *merged_config_ptree_;

	merged_config_ptree_ = std::make_unique<ptree>();
	*merged_config_ptree_ = merged_config();
	return *merged_config_ptree_;
}

const ptree& Config::default_config() const {
	std::lock_guard<decltype(default_config_mtx_)> default_config_mtx_lk(default_config_mtx_);
	if(default_config_ptree_) return *default_config_ptree_;

	default_config_ptree_ = std::make_unique<ptree>();

	// device_name
	default_config_ptree_->put("device_name", boost::asio::ip::host_name());
	// control
	default_config_ptree_->put("control.listen", "127.0.0.1:61345");
	// net
	default_config_ptree_->put("net.listen", "[::]:51345");
	// net.natpmp
	default_config_ptree_->put("net.natpmp.enabled", true);
	default_config_ptree_->put("net.natpmp.lifetime", 3600);
	// discovery.static
	default_config_ptree_->put("discovery.static.repeat_interval", 30);
	// discovery.multicast4
	default_config_ptree_->put("discovery.multicast4.enabled", true);
	default_config_ptree_->put("discovery.multicast4.local_ip", "0.0.0.0");
	default_config_ptree_->put("discovery.multicast4.ip", "239.192.152.144");
	default_config_ptree_->put("discovery.multicast4.port", 28914);
	default_config_ptree_->put("discovery.multicast4.repeat_interval", 30);
	// discovery.multicast6
	default_config_ptree_->put("discovery.multicast6.enabled", true);
	default_config_ptree_->put("discovery.multicast6.local_ip", "::");
	default_config_ptree_->put("discovery.multicast6.ip", "ff08::BD02");
	default_config_ptree_->put("discovery.multicast6.port", 28914);
	default_config_ptree_->put("discovery.multicast6.repeat_interval", 30);
	// discovery.bttracker
	default_config_ptree_->put("discovery.bttracker.enabled", true);
	default_config_ptree_->put("discovery.bttracker.num_want", 30);
	default_config_ptree_->put("discovery.bttracker.min_interval", 15);
	default_config_ptree_->put("discovery.bttracker.azureus_id", "-LV0001-");
	default_config_ptree_->put("discovery.bttracker.udp.reconnect_interval", 120);
	default_config_ptree_->put("discovery.bttracker.udp.packet_timeout", 10);
	// discovery.bttracker.trackers
	std::list<std::string> trackers_list;
	trackers_list.push_back("udp://tracker.openbittorrent.com:80");
	trackers_list.push_back("udp://open.demonii.com:1337");
	trackers_list.push_back("udp://tracker.coppersurfer.tk:6969");
	trackers_list.push_back("udp://tracker.leechers-paradise.org:6969");

	ptree trackers_ptree;
	for(auto tracker_url : trackers_list) {
		ptree tracker_ptree; tracker_ptree.put("", tracker_url);
		trackers_ptree.push_back({"", tracker_ptree});
	}

	default_config_ptree_->add_child("discovery.bttracker.trackers", trackers_ptree);
	// folder
	ptree folders_ptree;
	ptree folder_ptree;
	folder_ptree.put("index_event_timeout", 1000);
	folder_ptree.put("preserve_unix_attrib", false);
	folders_ptree.push_back({"", folder_ptree});

	default_config_ptree_->add_child("folders", folders_ptree);

	return *default_config_ptree_;
}

const ptree& Config::persistent_config() const {
	std::lock_guard<decltype(file_config_mtx_)> default_config_mtx_lk(file_config_mtx_);
	if(file_config_ptree_) return *file_config_ptree_;

	file_config_ptree_ = std::make_unique<ptree>();

	fs::ifstream options_fs(config_path_, std::ios::binary);
	if(options_fs){
		boost::property_tree::json_parser::read_json(options_fs, *file_config_ptree_);
	}
	return *file_config_ptree_;
}

ptree Config::merged_config() const {
	ptree default_conf = default_config();

	ptree merged_conf;
	merge_value(merged_conf, *file_config_ptree_, default_conf, "device_name");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "control.listen");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "net.listen");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "net.natpmp.enabled");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "net.natpmp.lifetime");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.static.repeat_interval");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.multicast4.enabled");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.multicast4.local_ip");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.multicast4.ip");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.multicast4.port");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.multicast4.repeat_interval");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.multicast6.enabled");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.multicast6.local_ip");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.multicast6.ip");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.multicast6.port");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.multicast6.repeat_interval");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.bttracker.enabled");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.bttracker.num_want");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.bttracker.min_interval");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.bttracker.azureus_id");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.bttracker.num_want");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.bttracker.udp.reconnect_interval");
	merge_value(merged_conf, *file_config_ptree_, default_conf, "discovery.bttracker.udp.packet_timeout");

	merge_child(merged_conf, *file_config_ptree_, default_conf, "discovery.bttracker.trackers");

	try {
		auto eqkey_range = file_config_ptree_->get_child("folders").equal_range("");
		ptree folders_ptree;
		for(auto it = eqkey_range.first; it != eqkey_range.second; it++) {
			// Trees for this folder
			ptree folder_ptree;
			ptree& config_folder_tree = it->second;
			ptree& default_folder_tree = default_conf.get_child("folders").equal_range("").first->second;

			// Non-default values
			merge_value(folder_ptree, config_folder_tree, default_folder_tree, "secret");
			merge_value(folder_ptree, config_folder_tree, default_folder_tree, "open_path");
			merge_value(folder_ptree, config_folder_tree, default_folder_tree, "block_path");
			merge_value(folder_ptree, config_folder_tree, default_folder_tree, "db_path");
			merge_value(folder_ptree, config_folder_tree, default_folder_tree, "asm_path");

			merge_child(folder_ptree, config_folder_tree, default_folder_tree, "ignore_paths");

			// Simply merge others
			merge_value(folder_ptree, config_folder_tree, default_folder_tree, "index_event_timeout");
			merge_value(folder_ptree, config_folder_tree, default_folder_tree, "preserve_unix_attrib");

			folders_ptree.push_back({"", folder_ptree});
		}
		merged_conf.add_child("folders", folders_ptree);
	}catch(boost::property_tree::ptree_bad_path& e) {
		// This is catched if "folders" don't exist
	}

	return merged_conf;
}

void Config::merge_value(ptree& merged_conf, const ptree& actual_conf, const ptree& default_conf, const std::string& key) const {
	auto optvalue_default = default_conf.get_optional<std::string>(key);
	auto optvalue_actual = actual_conf.get_optional<std::string>(key);
	if(optvalue_actual)
		merged_conf.put(key, optvalue_actual.value());
	else if(optvalue_default)
		merged_conf.put(key, optvalue_default.value());
}

void Config::merge_child(ptree& merged_conf, const ptree& actual_conf, const ptree& default_conf, const std::string& key) const {
	//merged_conf.put_child(key, actual_conf.get_child(key, default_conf.get_child(key)));
	auto optchild_default = default_conf.get_child_optional(key);
	auto optchild_actual = actual_conf.get_child_optional(key);
	if(optchild_actual)
		merged_conf.put_child(key, optchild_actual.value());
	else if(optchild_default)
		merged_conf.put_child(key, optchild_default.value());
}

} /* namespace librevault */
