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
#include "pch.h"
#include "util/Loggable.h"
#include <json/json.h>
#include <librevault/Secret.h>
#include <librevault/Meta.h>

namespace librevault {

class Config : protected Loggable {
public:
	~Config();

	struct paths_type {
		fs::path appdata_path, client_config_path, folders_config_path, log_path, key_path, cert_path;
	};

	boost::signals2::signal<void()> config_changed;

	static void init(fs::path appdata_path = fs::path()) {
		instance_ = std::unique_ptr<Config>(new Config(std::move(appdata_path)));
	}
	static Config* get() {
		if(!instance_) throw std::runtime_error("Config is not initialized yet");
		return instance_.get();
	}

	/* Getters and setters */
	const Json::Value& client() const {return client_;}
	void set_client(Json::Value client_conf);
	const Json::Value& client_defaults() const {return client_defaults_;} // For main

	const Json::Value& folders() const {return folders_;}
	void set_folders(Json::Value folders_conf);

	const paths_type& paths() const {return paths_;}

protected:
	Config(fs::path appdata_path);

	static std::unique_ptr<Config> instance_;

private:
	Json::Value client_, client_custom_, client_defaults_, folders_, folders_custom_, folders_defaults_;

	paths_type paths_;
	fs::path default_appdata_path();

	void make_defaults();
	void make_merged_client();
	void make_merged_folders();

	void load();
	void save();
};

} /* namespace librevault */
