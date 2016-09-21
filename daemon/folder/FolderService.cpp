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
#include "folder/fs/FSFolder.h"
#include "folder/fs/Indexer.h"
#include "util/log.h"
#include <boost/range/adaptor/map.hpp>

namespace librevault {

FolderService::FolderService() : ios_("FolderService") {
	for(auto& folder_config : Config::get()->folders())
		init_folder(folder_config);
}

FolderService::~FolderService() {stop();}

void FolderService::run() {ios_.start(std::thread::hardware_concurrency());}
void FolderService::stop() {
	hash_group_.clear();
	ios_.stop();
}

void FolderService::add_folder(Json::Value json_folder) {
	LOGFUNC();

	FolderParams params(json_folder);

	auto folders_copy = Config::get()->folders_custom();

	auto it = std::find_if(folders_copy.begin(), folders_copy.end(), [&](const Json::Value& v){
		return FolderParams(v).secret.get_Hash() == params.secret.get_Hash();
	});
	if(it == folders_copy.end()) {
		folders_copy.append(json_folder);
		Config::get()->set_folders(folders_copy);

		init_folder(params);
	}else
		throw samekey_error();
}

void FolderService::remove_folder(const Secret& secret) {
	LOGFUNC();

	auto folders_copy = Config::get()->folders_custom();
	for(Json::ArrayIndex i = 0; i < folders_copy.size(); i++) {
		if(FolderParams(folders_copy[i]).secret.get_Hash() == secret.get_Hash()) {
			deinit_folder(secret);

			Json::Value temp;
			folders_copy.removeIndex(i, &temp);
			break;
		}
	}

	Config::get()->set_folders(folders_copy);
}

void FolderService::init_folder(const FolderParams& params) {
	LOGFUNC();

	auto group_ptr = get_group(params.secret.get_Hash());
	if(!group_ptr) {
		group_ptr = std::make_shared<FolderGroup>(params, ios_.ios());
		hash_group_.insert({group_ptr->hash(), group_ptr});

		folder_added_signal(group_ptr);
	}else
		throw samekey_error();
}

void FolderService::deinit_folder(const Secret& secret) {
	LOGFUNC();

	auto group_ptr = get_group(secret.get_Hash());

	hash_group_.erase(secret.get_Hash());
	folder_removed_signal(group_ptr);
	LOGD("Group deinitialized: " << secret);
}

std::shared_ptr<FolderGroup> FolderService::get_group(const blob& hash) {
	auto it = hash_group_.find(hash);
	if(it != hash_group_.end())
		return it->second;
	return nullptr;
}

std::vector<std::shared_ptr<FolderGroup>> FolderService::groups() const {
	std::vector<std::shared_ptr<FolderGroup>> groups_list;
	for(auto group_ptr : hash_group_ | boost::adaptors::map_values)
		groups_list.push_back(group_ptr);
	return groups_list;
}

} /* namespace librevault */
