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
#include "FolderService.h"
#include "FolderGroup.h"
#include "control/Config.h"
#include "control/StateCollector.h"
#include "folder/meta/Indexer.h"
#include "util/log.h"
#include <librevault/crypto/Hex.h>
#include <boost/range/adaptor/map.hpp>

namespace librevault {

FolderService::FolderService(StateCollector& state_collector, QObject* parent) : QObject(parent),
	bulk_ios_("FolderService_bulk"),
	serial_ios_("FolderService_serial"),
	state_collector_(state_collector) {
	LOGFUNC();
}

FolderService::~FolderService() {
	LOGFUNC();
	stop();
	LOGFUNCEND();
}

void FolderService::run() {
	serial_ios_.start(1);
	bulk_ios_.start(std::max(std::thread::hardware_concurrency(), 1u));

	QTimer::singleShot(0, this, [this] {
		for(auto& folder_config : Config::get()->folders())
			init_folder(folder_config.second);
	});

	connect(Config::get(), &Config::folderAdded, this, &FolderService::init_folder);
	connect(Config::get(), &Config::folderRemoved, this, &FolderService::deinit_folder);
}

void FolderService::stop() {
	for(auto& hash : hash_group_.keys())
		deinit_folder(hash);

	bulk_ios_.stop();
	serial_ios_.stop();
}

void FolderService::init_folder(const FolderParams& params) {
	LOGFUNC();
	auto group_ptr = std::make_shared<FolderGroup>(params, state_collector_, bulk_ios_.ios(), serial_ios_.ios(), this);
	hash_group_[group_ptr->hash()] = group_ptr;
	groups_[group_ptr->folderid()] = group_ptr.get();

	emit folderAdded(group_ptr);
	LOGD("Folder initialized: " << group_ptr->folderid().toHex().toStdString());
}

void FolderService::deinit_folder(const blob& folder_hash) {
	LOGFUNC();
	auto group_ptr = get_group(folder_hash);
	emit folderRemoved(group_ptr);

	hash_group_.remove(folder_hash);

	QByteArray folderid((char*)folder_hash.data(), folder_hash.size());
	groups_.remove(folderid);
	LOGD("Folder deinitialized: " << folderid.toHex().toStdString());
}

std::shared_ptr<FolderGroup> FolderService::get_group(const blob& hash) {
	auto it = hash_group_.find(hash);
	if(it != hash_group_.end())
		return it.value();
	throw invalid_group();
}

FolderGroup* FolderService::getGroup(const QByteArray& folderid) {
	return groups_.value(folderid, nullptr);
}

} /* namespace librevault */
