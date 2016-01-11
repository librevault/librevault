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
#include "../pch.h"
#include "Loggable.h"

namespace librevault {

class Config : protected Loggable {
public:
	Config(Loggable& parent_loggable, fs::path config_path);
	~Config();

	const ptree& general_config() const;
	const ptree& default_config() const;
	const ptree& persistent_config() const;

	enum ConfigType {
		GENERAL,
		DEFAULT,
		PERSISTENT
	};

	template<class T>
	T get(const std::string path, ConfigType type = GENERAL) {
		switch(type) {
			case GENERAL: return general_config().get<T>(path);
			case DEFAULT: return default_config().get<T>(path);
			case PERSISTENT: return persistent_config().get<T>(path);
		}
		return T();
	}

	const ptree& get_child(const std::string path, ConfigType type = GENERAL) {
		switch(type) {
			case GENERAL: return general_config().get_child(path);
			case DEFAULT: return default_config().get_child(path);
			case PERSISTENT: return persistent_config().get_child(path);
		}
		throw;
	}
private:
	fs::path config_path_;

	mutable std::mutex file_config_mtx_, default_config_mtx_, merged_config_mtx_;
	mutable std::unique_ptr<ptree> file_config_ptree_, default_config_ptree_, merged_config_ptree_;

	ptree merged_config() const;
	void merge_value(ptree& merged_conf, const ptree& actual_conf, const ptree& default_conf, const std::string& key) const;
	void merge_child(ptree& merged_conf, const ptree& actual_conf, const ptree& default_conf, const std::string& key) const;
};

} /* namespace librevault */
