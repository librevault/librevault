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
#include "Config.h"
#include "Paths.h"
#include "util/blob.h"
#include <librevault/Secret.h>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QSaveFile>

Q_LOGGING_CATEGORY(log_config, "config")

namespace librevault {

Config::Config() {
	make_defaults();
	load();

	connect(this, &Config::configChanged, this, [this](QString key, QVariant value){
		qCInfo(log_config) << "Global var" << key << "is set to" << value;
	});
	connect(this, &Config::configChanged, this, &Config::save);
}

Config::~Config() {
	save();
}

Config* Config::instance_ = nullptr;

QVariant Config::global_get(QString name) {
	return (globals_custom_.contains(name) ? globals_custom_[name] : globals_defaults_[name]).toVariant();
}

void Config::global_set(QString name, QVariant value) {
	globals_custom_[name] = QJsonValue::fromVariant(value);
	emit configChanged(name, value);
}

void Config::global_unset(QString name) {
	globals_custom_.remove(name);
	emit configChanged(name, global_get(name));
}

QJsonObject Config::export_globals_custom() const {
	return globals_custom_;
}

QJsonObject Config::export_globals() const {
	return make_merged(globals_custom_, globals_defaults_);
}

void Config::import_globals(QJsonObject globals_conf) {
	QStringList all_keys = globals_custom_.keys() + globals_conf.keys();
	QSet<QString> changed_keys;

	foreach(const QString& key, all_keys) {
		if(globals_custom_.value(key) != globals_conf.value(key)) {
			changed_keys.insert(key);
		}
	}

	globals_custom_ = globals_conf;

	// Notify other components
	foreach(const QString& key, all_keys) {
		emit configChanged(key, global_get(key));
	}
}

void Config::folder_add(QJsonObject folder_config) {
	QByteArray folderid = conv_bytearray(Secret(folder_config["secret"].toString().toStdString()).get_Hash());
	if(folders_custom_.contains(folderid))
		throw samekey_error();
	folders_custom_.insert(folderid, folder_config);
	emit folderAdded(make_merged(folder_config, folders_defaults_));
	save();
}

void Config::folder_remove(QByteArray folderid) {
	emit folderRemoved(QByteArray((const char*)folderid.data(), folderid.size()));
	folders_custom_.remove(folderid);
	save();
}

QJsonObject Config::folder_get(QByteArray folderid) {
	return folders_custom_.contains(folderid) ? folders_custom_[folderid] : QJsonObject();
}

QMap<QByteArray, QJsonObject> Config::folders() const {
	QMap<QByteArray, QJsonObject> folders_merged;
	foreach(const QByteArray& folderid, folders_custom_.keys()) {
		folders_merged[folderid] = make_merged(folders_custom_[folderid], folders_defaults_);
	}
	return folders_merged;
}

QJsonArray Config::export_folders_custom() const {
	QJsonArray folders_merged;
	foreach(const QJsonObject& folder_params, folders_custom_.values()) {
		folders_merged.append(folder_params);
	}
	return folders_merged;
}

QJsonArray Config::export_folders() const {
	QJsonArray folders_merged;
	foreach(const QJsonObject& folder_params, folders().values()) {
		folders_merged.append(folder_params);
	}
	return folders_merged;
}

void Config::import_folders(QJsonArray folders_conf) {
	foreach(const QByteArray& folderid, folders_custom_.keys()) {
		folder_remove(folderid);
	}
	foreach(const QJsonValue& folder_params_v, folders_conf) {
		folder_add(folder_params_v.toObject());
	}
}

void Config::make_defaults() {
	QJsonDocument globals_defaults, folders_defaults;

	{
		QFile globals_defaults_f(":/config/globals.json");
		globals_defaults_f.open(QIODevice::ReadOnly);

		globals_defaults = QJsonDocument::fromJson(globals_defaults_f.readAll());
		Q_ASSERT(!globals_defaults.isEmpty());
	}

	{
		QFile folders_defaults_f(":/config/folders.json");
		folders_defaults_f.open(QIODevice::ReadOnly);

		folders_defaults = QJsonDocument::fromJson(folders_defaults_f.readAll());
		Q_ASSERT(!folders_defaults.isEmpty());
	}

	globals_defaults_ = globals_defaults.object();
	globals_defaults_["client_name"] = QSysInfo::machineHostName();

	folders_defaults_ = folders_defaults.object();
}

QJsonObject Config::make_merged(QJsonObject custom_value, QJsonObject default_value) const {
	QStringList all_keys = custom_value.keys() + default_value.keys();
	all_keys.removeDuplicates();

	QJsonObject merged;
	foreach(const QString& key, all_keys) {
		merged[key] = custom_value.contains(key) ? custom_value[key] : default_value[key];
	}
	return merged;
}

void Config::load() {
	QFile globals_f(QString::fromStdWString(Paths::get()->client_config_path.wstring()));
	if(globals_f.open(QIODevice::ReadOnly)) {
		QJsonObject globals_load = QJsonDocument::fromJson(globals_f.readAll()).object();
		qCDebug(log_config) << "Loading global configuration from:" << globals_f.fileName();
		import_globals(globals_load);
	}else
		qCWarning(log_config) << "Could not load global configuration from:" << globals_f.fileName();

	QFile folders_f(QString::fromStdWString(Paths::get()->folders_config_path.wstring()));
	if(folders_f.open(QIODevice::ReadOnly)) {
		QJsonArray folders_load = QJsonDocument::fromJson(folders_f.readAll()).array();
		qCDebug(log_config) << "Loading folder configuration from:" << folders_f.fileName();
		import_folders(folders_load);
	}else
		qCWarning(log_config) << "Could not load folder configuration from:" << folders_f.fileName();
}

void Config::save() {
	{
		QSaveFile globals_f(QString::fromStdWString(Paths::get()->client_config_path.wstring()));
		if(globals_f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			qCDebug(log_config) << "Saving global configuration to:" << globals_f.fileName();
			globals_f.write(QJsonDocument(export_globals_custom()).toJson(QJsonDocument::Compact));
		}
		if(globals_f.isOpen() && globals_f.commit())
			qCDebug(log_config) << "Saved global configuration to:" << globals_f.fileName();
		else
			qCWarning(log_config) << "Could not save global configuration to:" << globals_f.fileName();
	}

	{
		QSaveFile folders_f(QString::fromStdWString(Paths::get()->folders_config_path.wstring()));
		if(folders_f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			qCDebug(log_config) << "Saving folder configuration to:" << folders_f.fileName();
			folders_f.write(QJsonDocument(export_folders_custom()).toJson(QJsonDocument::Compact));
		}
		if(folders_f.isOpen() && folders_f.commit())
			qCDebug(log_config) << "Saved folder configuration to:" << folders_f.fileName();
		else
			qCWarning(log_config) << "Could not save folder configuration to:" << folders_f.fileName();
	}
}

} /* namespace librevault */
