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
#include <codecvt>
#include "util/file_util.h"

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

	paths_.client_config_path = paths_.appdata_path / "globals.json";
	paths_.folders_config_path = paths_.appdata_path / "folders.json";
	paths_.log_path = paths_.appdata_path / "librevault.log";
	paths_.key_path = paths_.appdata_path / "key.pem";
	paths_.cert_path = paths_.appdata_path / "cert.pem";

	make_defaults();
	load();

	config_changed.connect([this](){save();});
}

Config::~Config() {
	save();
}

std::unique_ptr<Config> Config::instance_ = nullptr;

void Config::set_globals(Json::Value globals_conf) {
	globals_custom_ = std::move(globals_conf);
	make_merged_globals();
	config_changed();
}

void Config::set_folders(Json::Value folders_conf) {
	folders_custom_ = std::move(folders_conf);
	make_merged_folders();
	config_changed();
}

void Config::make_defaults() {
	/* globals_defaults_ */
	globals_defaults_.clear();
	globals_defaults_["client_name"] = boost::asio::ip::host_name();
	globals_defaults_["control_listen"] = "[::]:42346";
	globals_defaults_["p2p_listen"] = "[::]:42345";
	globals_defaults_["p2p_download_slots"] = 10;
	globals_defaults_["p2p_request_timeout"] = 10;
	globals_defaults_["p2p_block_size"] = 32768;
	globals_defaults_["natpmp_enabled"] = true;
	globals_defaults_["natpmp_lifetime"] = 3600;
	globals_defaults_["predef_repeat_interval"] = 30;
	globals_defaults_["multicast4_enabled"] = true;
	globals_defaults_["multicast4_group"] = "239.192.152.144:28914";
	globals_defaults_["multicast4_repeat_interval"] = 30;
	globals_defaults_["multicast6_enabled"] = true;
	globals_defaults_["multicast6_group"] = "[ff08::BD02]:28914";
	globals_defaults_["multicast6_repeat_interval"] = 30;
	globals_defaults_["bttracker_enabled"] = true;
	globals_defaults_["bttracker_num_want"] = 30;
	globals_defaults_["bttracker_min_interval"] = 15;
	globals_defaults_["bttracker_azureus_id"] = "-LV0001-";
	globals_defaults_["bttracker_reconnect_interval"] = 30;
	globals_defaults_["bttracker_packet_timeout"] = 10;

	//globals_defaults_["bttracker_discovery_trackers"].append("udp://discovery.librevault.com:42340");  // TODO: Soon, guys!
	globals_defaults_["bttracker_trackers"].append("udp://tracker.openbittorrent.com:80");
	globals_defaults_["bttracker_trackers"].append("udp://open.demonii.com:1337");
	globals_defaults_["bttracker_trackers"].append("udp://tracker.coppersurfer.tk:6969");
	globals_defaults_["bttracker_trackers"].append("udp://tracker.leechers-paradise.org:6969");
	globals_defaults_["bttracker_trackers"].append("udp://tracker.opentrackr.org:1337");

	/* folders_defaults_ */
	folders_defaults_.clear();
	folders_defaults_["index_event_timeout"] = 1000;
	folders_defaults_["preserve_unix_attrib"] = false;
	folders_defaults_["preserve_windows_attrib"] = false;
	folders_defaults_["preserve_symlinks"] = false;
	folders_defaults_["normalize_unicode"] = true;
	folders_defaults_["chunk_strong_hash_type"] = 0;
	folders_defaults_["full_rescan_interval"] = 60;
}

void Config::make_merged_globals() {
	globals_.clear();
	for(auto name : globals_defaults_.getMemberNames())
		globals_[name] = globals_custom_.get(name, globals_defaults_[name]);
}

void Config::make_merged_folders() {
	folders_ = folders_custom_;
	for(auto& folder_item : folders_)
		for(auto name : folders_defaults_.getMemberNames())
			if(!folder_item.isMember(name))
				folder_item[name] = folders_defaults_[name];
}

void Config::load() {
	file_wrapper globals_f(paths_.client_config_path, "rb");
	file_wrapper folders_f(paths_.folders_config_path, "rb");

	Json::Reader r;
	r.parse(globals_f.ios(), globals_custom_);
	r.parse(folders_f.ios(), folders_custom_);

	set_globals(globals_custom_);
	set_folders(folders_custom_);
}

void Config::save() {
	file_wrapper globals_f(paths_.client_config_path, "wb");
	file_wrapper folders_f(paths_.folders_config_path, "wb");

	globals_f.ios() << globals_custom_.toStyledString();
	folders_f.ios() << folders_custom_.toStyledString();
}

#if BOOST_OS_MACOS == BOOST_VERSION_NUMBER_NOT_AVAILABLE

fs::path Config::default_appdata_path() {   // TODO: separate to multiple files
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
