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

#if BOOST_OS_LINUX || BOOST_OS_BSD || BOOST_OS_UNIX
#   include <pwd.h>
#elif BOOST_OS_WINDOWS
#	include <shlobj.h>
#endif

namespace librevault {

Config::Config(fs::path appdata_path) {
	if(appdata_path.empty())
		paths_.appdata_path = default_appdata_path();
	else
		paths_.appdata_path = std::move(appdata_path);

	fs::create_directories(paths_.appdata_path);

	paths_.client_config_path = paths_.appdata_path / "client.json";
	paths_.folders_config_path = paths_.appdata_path / "folders.json";
	paths_.log_path = paths_.appdata_path / "librevault.log";
	paths_.key_path = paths_.appdata_path / "key.pem";
	paths_.cert_path = paths_.appdata_path / "cert.pem";

	make_defaults();
	load();
}

Config::~Config() {
	save();
}

std::unique_ptr<Config> Config::instance_ = nullptr;

void Config::set_client(Json::Value client_conf) {
	client_custom_ = std::move(client_conf);
	make_merged_client();
	config_changed();
}

void Config::set_folders(Json::Value folders_conf) {
	folders_custom_ = std::move(folders_conf);
	make_merged_folders();
	config_changed();
}

void Config::make_defaults() {
	/* client_defaults_ */
	client_defaults_.clear();
	client_defaults_["client_name"] = boost::asio::ip::host_name();
	client_defaults_["control_listen"] = "[::]:42346";
	client_defaults_["p2p_listen"] = "[::]:42345";
	client_defaults_["p2p_download_slots"] = 10;
	client_defaults_["p2p_request_timeout"] = 10;
	client_defaults_["p2p_block_size"] = 32768;
	client_defaults_["natpmp_enabled"] = true;
	client_defaults_["natpmp_lifetime"] = 3600;
	client_defaults_["predef_repeat_interval"] = 30;
	client_defaults_["multicast4_enabled"] = true;
	client_defaults_["multicast4_group"] = "239.192.152.144:28914";
	client_defaults_["multicast4_repeat_interval"] = 30;
	client_defaults_["multicast6_enabled"] = true;
	client_defaults_["multicast6_group"] = "[ff08::BD02]:28914";
	client_defaults_["multicast6_repeat_interval"] = 30;
	client_defaults_["bttracker_enabled"] = true;
	client_defaults_["bttracker_num_want"] = 30;
	client_defaults_["bttracker_min_interval"] = 15;
	client_defaults_["bttracker_azureus_id"] = "-LV0001-";
	client_defaults_["bttracker_reconnect_interval"] = 30;
	client_defaults_["bttracker_packet_timeout"] = 10;

	//client_defaults_["bttracker_discovery_trackers"].append("udp://discovery.librevault.com:42340");  // TODO: Soon, guys!
	client_defaults_["bttracker_trackers"].append("udp://tracker.openbittorrent.com:80");
	client_defaults_["bttracker_trackers"].append("udp://open.demonii.com:1337");
	client_defaults_["bttracker_trackers"].append("udp://tracker.coppersurfer.tk:6969");
	client_defaults_["bttracker_trackers"].append("udp://tracker.leechers-paradise.org:6969");
	client_defaults_["bttracker_trackers"].append("udp://tracker.opentrackr.org:1337");

	/* folders_defaults_ */
	folders_defaults_.clear();
	folders_defaults_["index_event_timeout"] = 1000;
	folders_defaults_["preserve_unix_attrib"] = false;
	folders_defaults_["preserve_windows_attrib"] = false;
	folders_defaults_["preserve_symlinks"] = false;
	folders_defaults_["chunk_strong_hash_type"] = 0;
}

void Config::make_merged_client() {
	client_.clear();
	for(auto name : client_defaults_.getMemberNames())
		client_[name] = client_custom_.get(name, client_defaults_[name]);
}

void Config::make_merged_folders() {
	folders_ = folders_custom_;
	for(auto& folder_item : folders_)
		for(auto name : folders_defaults_.getMemberNames())
			if(!folder_item.isMember(name))
				folder_item[name] = folders_defaults_[name];
}

void Config::load() {
	fs::ifstream client_ifs(paths_.client_config_path, std::ifstream::binary);
	fs::ifstream folders_ifs(paths_.folders_config_path, std::ifstream::binary);

	Json::Reader r;
	r.parse(client_ifs, client_custom_);
	r.parse(folders_ifs, folders_custom_);

	set_client(client_custom_);
	set_folders(folders_custom_);
}

void Config::save() {
	fs::ofstream client_ofs(paths_.client_config_path, std::ios_base::trunc | std::ifstream::binary);
	fs::ofstream folders_ofs(paths_.folders_config_path, std::ios_base::trunc | std::ifstream::binary);

	client_ofs << client_custom_.toStyledString();
	folders_ofs << folders_custom_.toStyledString();
}

#if BOOST_OS_MACOS == BOOST_VERSION_NUMBER_NOT_AVAILABLE

fs::path Config::default_appdata_path() {
#if BOOST_OS_WINDOWS
	PWSTR appdata_path;
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appdata_path);
	fs::path folder_path = fs::path(appdata_path) / Version::current().name();
	CoTaskMemFree(appdata_path);

	return folder_path;
#elif BOOST_OS_LINUX || BOOST_OS_UNIX
	if(char* xdg_ptr = getenv("XDG_CONFIG_HOME"))
		return fs::path(xdg_ptr) / Version::current().name();
	if(char* home_ptr = getenv("HOME"))
		return fs::path(home_ptr) / ".config" / Version::current().name();
	if(char* home_ptr = getpwuid(getuid())->pw_dir)
		return fs::path(home_ptr) / ".config" / Version::current().name();
	return fs::path("/etc/xdg") / Version::current().name();
#else
	// Well, we will add some Android values here. And, maybe, others.
	return fs::path(getenv("HOME")) / Version::current().name();
#endif
}

#endif	// BOOST_OS_MACOS == BOOST_VERSION_NUMBER_NOT_AVAILABLE

} /* namespace librevault */
