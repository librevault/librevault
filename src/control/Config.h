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
	std::string getDevice_name() const {return current.device_name;}
	void setDevice_name(const std::string& device_name) {current.device_name = device_name; save();}
	url getControl_listen() const {return current.control_listen;}
	void setControl_listen(const url& control_listen) {current.control_listen = control_listen; save();}
	url getNet_listen() const {return current.net_listen;}
	void setNet_listen(const url& net_listen) {current.net_listen = net_listen; save();}
	bool isNatpmp_enabled() const {return current.natpmp_enabled;}
	void setNatpmp_enabled(bool net_natpmp_enabled) {current.natpmp_enabled = net_natpmp_enabled; save();}
	seconds getNatpmp_lifetime() const {return current.natpmp_lifetime;}
	void setNatpmp_lifetime(const seconds& net_natpmp_lifetime) {current.natpmp_lifetime = net_natpmp_lifetime; save();}
	seconds getStatic_repeat_interval() const {return current.static_repeat_interval;}
	void setStatic_repeat_interval(const seconds& discovery_static_repeat_interval) {current.static_repeat_interval = discovery_static_repeat_interval; save();}
	bool isMulticast4_enabled() const {return current.multicast4_enabled;}
	void setMulticast4_enabled(bool discovery_multicast4_enabled) {current.multicast4_enabled = discovery_multicast4_enabled; save();}
	address_v4 getMulticast4_local_ip() const {return current.multicast4_local_ip;}
	void setMulticast4_local_ip(const address_v4& discovery_multicast4_local_ip) {current.multicast4_local_ip = discovery_multicast4_local_ip; save();}
	address_v4 getMulticast4_ip() const {return current.multicast4_ip;}
	void setMulticast4_ip(const address_v4& discovery_multicast4_ip) {current.multicast4_ip = discovery_multicast4_ip; save();}
	uint16_t getMulticast4_port() const {return current.multicast4_port;}
	void setMulticast4_port(uint16_t discovery_multicast4_port) {current.multicast4_port = discovery_multicast4_port; save();}
	seconds getMulticast4_repeat_interval() const {return current.multicast4_repeat_interval;}
	void setMulticast4_repeat_interval(const seconds& discovery_multicast4_repeat_interval) {current.multicast4_repeat_interval = discovery_multicast4_repeat_interval; save();}
	bool isMulticast6_enabled() const {return current.multicast6_enabled;}
	void setMulticast6_enabled(bool discovery_multicast6_enabled) {current.multicast6_enabled = discovery_multicast6_enabled; save();}
	address_v6 getMulticast6_local_ip() const {return current.multicast6_local_ip;}
	void setMulticast6_local_ip(const address_v6& discovery_multicast6_local_ip) {current.multicast6_local_ip = discovery_multicast6_local_ip; save();}
	address_v6 getMulticast6_ip() const {return current.multicast6_ip;}
	void setMulticast6_ip(const address_v6& discovery_multicast6_ip) {current.multicast6_ip = discovery_multicast6_ip; save();}
	uint16_t getMulticast6_port() const {return current.multicast6_port;}
	void setMulticast6_port(uint16_t discovery_multicast6_port) {current.multicast6_port = discovery_multicast6_port; save();}
	seconds getMulticast6_repeat_interval() const {return current.multicast6_repeat_interval;}
	void setMulticast6_repeat_interval(const seconds& discovery_multicast6_repeat_interval) {current.multicast6_repeat_interval = discovery_multicast6_repeat_interval; save();}
	bool isBttracker_enabled() const {return current.bttracker_enabled;}
	void setBttracker_enabled(bool discovery_bttracker_enabled) {current.bttracker_enabled = discovery_bttracker_enabled; save();}
	unsigned int getBttracker_num_want() const {return current.bttracker_num_want;}
	void setBttracker_num_want(unsigned int discovery_bttracker_num_want) {current.bttracker_num_want = discovery_bttracker_num_want; save();}
	seconds getBttracker_min_interval() const {return current.bttracker_min_interval;}
	void setBttracker_min_interval(const seconds& discovery_bttracker_min_interval) {current.bttracker_min_interval = discovery_bttracker_min_interval; save();}
	std::string getBttracker_azureus_id() const {return current.bttracker_azureus_id;}
	void setBttracker_azureus_id(const std::string& discovery_bttracker_azureus_id) {current.bttracker_azureus_id = discovery_bttracker_azureus_id; save();}
	seconds getBttracker_udp_reconnect_interval() const {return current.bttracker_udp_reconnect_interval;}
	void setBttracker_udp_reconnect_interval(const seconds& discovery_bttracker_udp_reconnect_interval) {current.bttracker_udp_reconnect_interval = discovery_bttracker_udp_reconnect_interval; save();}
	seconds getBttracker_udp_packet_timeout() const {return current.bttracker_udp_packet_timeout;}
	void setBttracker_udp_packet_timeout(const seconds& discovery_bttracker_udp_packet_timeout) {current.bttracker_udp_packet_timeout = discovery_bttracker_udp_packet_timeout; save();}
	std::vector<url> getBttracker_trackers() const {return current.bttracker_trackers;}
	void setBttracker_trackers(const std::vector<url>& discovery_bttracker_trackers) {current.bttracker_trackers = discovery_bttracker_trackers; save();}
public:
	Config(Loggable& parent_loggable, fs::path config_path);
	~Config();

	ptree get_ptree() const;
	void apply_ptree(const ptree& pt);

	void save();

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

	void add_folder(FolderConfig folder_config);
	void remove_folder(Secret secret);

	std::vector<FolderConfig> folders() const {return current.folders;}

private:
	fs::path config_path_;

	struct config_type {
		std::string device_name;
		url control_listen;
		url net_listen;
		bool natpmp_enabled;
		seconds natpmp_lifetime;
		seconds static_repeat_interval;
		bool multicast4_enabled;
		address_v4 multicast4_local_ip;
		address_v4 multicast4_ip;
		uint16_t multicast4_port;
		seconds multicast4_repeat_interval;
		bool multicast6_enabled;
		address_v6 multicast6_local_ip;
		address_v6 multicast6_ip;
		uint16_t multicast6_port;
		seconds multicast6_repeat_interval;
		bool bttracker_enabled;
		unsigned bttracker_num_want;
		seconds bttracker_min_interval;
		std::string bttracker_azureus_id;
		seconds bttracker_udp_reconnect_interval;
		seconds bttracker_udp_packet_timeout;
		std::vector<url> bttracker_trackers;

		std::vector<FolderConfig> folders;
	};

	config_type current, defaults;

	config_type convert_pt(const ptree& pt, const config_type& base) const;
	ptree convert_pt(const config_type& config) const;

	void init_default_config();
};

} /* namespace librevault */
