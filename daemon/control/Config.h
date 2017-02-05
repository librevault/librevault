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
#include "util/blob.h"
#include "util/log.h"
#include <json/json.h>
#include <QObject>

namespace librevault {

class Config : public QObject {
	Q_OBJECT
	LOG_SCOPE("Config");
public:
	~Config();

	/* Exceptions */
	struct samekey_error : std::runtime_error {
		samekey_error() : std::runtime_error("Multiple directories with the same key (or derived from the same key) are not supported") {}
	};

	static Config* get() {
		if(!instance_)
			instance_ = new Config();
		return instance_;
	}
	static void deinit() {
		delete instance_;
		instance_ = nullptr;
	}

	/* Global configuration */
	Json::Value global_get(const std::string& name);
	void global_set(const std::string& name, Json::Value value);
	void global_unset(const std::string& name);

	Json::Value export_globals_custom() const;
	Json::Value export_globals() const;
	void import_globals(Json::Value globals_conf);

	/* Folder configuration */
	void folder_add(Json::Value folder_config);
	void folder_remove(const blob& folderid);

	Json::Value folder_get(const blob& folderid);
	std::map<blob, Json::Value> folders() const;

	Json::Value export_folders_custom() const;
	Json::Value export_folders() const;
	void import_folders(Json::Value folders_conf);

signals:
	void configChanged(std::string key, Json::Value value);
	void folderAdded(Json::Value fconfig);
	void folderRemoved(QByteArray folderid);

protected:
	Config();

	static Config* instance_;

private:
	Json::Value globals_custom_, globals_defaults_, folders_defaults_;
	std::map<blob, Json::Value> folders_custom_;

	void make_defaults();

	Json::Value make_merged(const Json::Value& custom_value, const Json::Value& default_value) const;

	// File config
	void load();
	void save();
};

} /* namespace librevault */
