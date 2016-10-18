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
#include "IgnoreList.h"
#include "control/FolderParams.h"
#include "folder/PathNormalizer.h"
#include "util/log.h"
#include "util/regex_escape.h"

namespace librevault {

IgnoreList::IgnoreList(const FolderParams& params, PathNormalizer& path_normalizer) : params_(params), path_normalizer_(path_normalizer) {
	set_ignored(params_.ignore_paths);

	LOGD("IgnoreList initialized");
}

bool IgnoreList::is_ignored(const std::string& relpath) const {
	std::unique_lock<std::mutex> lk(ignored_paths_mtx_);
	for(auto& ignored_path : ignored_paths_) {
		if(std::regex_match(relpath, ignored_path.second)) return true;
	}
	return false;
}

void IgnoreList::add_ignored(const std::string& relpath) {
	std::unique_lock<std::mutex> lk(ignored_paths_mtx_);
	if(!relpath.empty()) {
		std::regex relpath_regex(relpath, std::regex::icase | std::regex::optimize | std::regex::collate);
		ignored_paths_.insert({relpath, std::move(relpath_regex)});
		LOGD("Added to IgnoreList: " << relpath);
	}
}

void IgnoreList::remove_ignored(const std::string& relpath) {
	std::lock_guard<std::mutex> lk(ignored_paths_mtx_);
	ignored_paths_.erase(relpath);
	LOGD("Removed from IgnoreList: " << relpath);
}

void IgnoreList::set_ignored(const std::vector<std::string>& ignored_paths) {
	ignored_paths_.clear();

	// Config paths
	for(auto path : params_.ignore_paths) {
		add_ignored(path);
	}

	// Predefined paths
	add_ignored(regex_escape(path_normalizer_.normalize_path(params_.system_path)) + R"((?:\/(?:.*))?)");
}

} /* namespace librevault */
