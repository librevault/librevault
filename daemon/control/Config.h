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
#include <json/json.h>
#include <boost/filesystem/path.hpp>
#include <boost/signals2/signal.hpp>
#include <librevault/Secret.h>
#include <librevault/Meta.h>

namespace librevault {

class Config {
public:
	~Config();

	struct paths_type {
		boost::filesystem::path appdata_path, client_config_path, folders_config_path, log_path, key_path, cert_path, dht_session_path;
	};

	boost::signals2::signal<void(std::string)> config_changed;

	static void init(boost::filesystem::path appdata_path = boost::filesystem::path()) {
		instance_ = std::unique_ptr<Config>(new Config(std::move(appdata_path)));
	}
	static Config* get() {
		if(!instance_) throw std::runtime_error("Config is not initialized yet");
		return instance_.get();
	}

	/* Getters and setters */
	Json::Value global_get(const std::string& name);
	void global_set(const std::string& name, Json::Value value);
	void global_unset(const std::string& name);

	Json::Value globals() const;
	void set_globals(Json::Value globals_conf);
	const Json::Value& globals_defaults() const {return globals_defaults_;} // For main

	const Json::Value& folders_custom() const {return folders_custom_;}
	const Json::Value& folders() const {return folders_;}
	void set_folders(Json::Value folders_conf);

	const paths_type& paths() const {return paths_;}

protected:
	Config(boost::filesystem::path appdata_path);

	static std::unique_ptr<Config> instance_;

private:
	Json::Value globals_, globals_custom_, globals_defaults_, folders_, folders_custom_, folders_defaults_;

	paths_type paths_;
	boost::filesystem::path default_appdata_path();

	void make_defaults();

	Json::Value make_merged(const Json::Value& custom_value, const Json::Value& default_value) const;

	void make_merged_folders();

	// File config
	void load();
	void save();
};

} /* namespace librevault */
