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
	state_collector_(state_collector),
	init_queue_(serial_ios_.ios()) {
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

	init_queue_.post([this] {
		for(auto& folder_config : Config::get()->folders())
			init_folder(folder_config.second);
	});

	connect(Config::get(), &Config::folderAdded, this, [this](Json::Value json_params){
		init_queue_.post([this, json_params]{
			init_folder(json_params);
		});
	});
	connect(Config::get(), &Config::folderRemoved, this, [this](blob folderid){
		init_queue_.post([this, folderid]{
			deinit_folder(folderid);
		});
	});
}

void FolderService::stop() {
	init_queue_.post([this] {
		std::vector<blob> hashes;
		hashes.reserve(hash_group_.size());
		for(auto& hash : hash_group_ | boost::adaptors::map_keys)
			hashes.push_back(hash);

		for(auto& hash : hashes)
			deinit_folder(hash);
	}, true);
	init_queue_.stop();

	bulk_ios_.stop();
	serial_ios_.stop();
}

void FolderService::init_folder(const FolderParams& params) {
	LOGFUNC();
	auto group_ptr = std::make_shared<FolderGroup>(params, state_collector_, bulk_ios_.ios(), serial_ios_.ios());
	hash_group_[group_ptr->hash()] = group_ptr;

	emit folderAdded(group_ptr);
	LOGD("Folder initialized: " << crypto::Hex().to_string(params.secret.get_Hash()));
}

void FolderService::deinit_folder(const blob& folder_hash) {
	LOGFUNC();
	auto group_ptr = get_group(folder_hash);
	emit folderRemoved(group_ptr);

	hash_group_.erase(folder_hash);
	LOGD("Folder deinitialized: " << crypto::Hex().to_string(folder_hash));
}

std::shared_ptr<FolderGroup> FolderService::get_group(const blob& hash) {
	auto it = hash_group_.find(hash);
	if(it != hash_group_.end())
		return it->second;
	throw invalid_group();
}

} /* namespace librevault */
