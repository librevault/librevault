/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "IgnoreList.h"
#include "FSFolder.h"

namespace librevault {

IgnoreList::IgnoreList(FSFolder& dir) : Loggable(dir), dir_(dir) {
	set_ignored(dir_.params().ignore_paths);

	log_->debug() << log_tag() << "IgnoreList initialized";
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
		log_->debug() << log_tag() << "Added to IgnoreList: " << relpath;
	}
}

void IgnoreList::remove_ignored(const std::string& relpath) {
	std::lock_guard<std::mutex> lk(ignored_paths_mtx_);
	ignored_paths_.erase(relpath);
	log_->debug() << log_tag() << "Removed from IgnoreList: " << relpath;
}

void IgnoreList::set_ignored(const std::vector<std::string>& ignored_paths) {
	ignored_paths_.clear();

	// Config paths
	for(auto path : dir_.params().ignore_paths) {
		add_ignored(path);
	}

	// Predefined paths
	add_ignored(regex_escape(dir_.normalize_path(dir_.system_path())) + R"((?:\/(?:.*))?)");
}

std::string IgnoreList::regex_escape(std::string str_to_escape) {
	boost::replace_all(str_to_escape, "\\", "\\\\");
	boost::replace_all(str_to_escape, "^", "\\^");
	boost::replace_all(str_to_escape, ".", "\\.");
	boost::replace_all(str_to_escape, "$", "\\$");
	boost::replace_all(str_to_escape, "|", "\\|");
	boost::replace_all(str_to_escape, "(", "\\(");
	boost::replace_all(str_to_escape, ")", "\\)");
	boost::replace_all(str_to_escape, "[", "\\[");
	boost::replace_all(str_to_escape, "]", "\\]");
	boost::replace_all(str_to_escape, "*", "\\*");
	boost::replace_all(str_to_escape, "+", "\\+");
	boost::replace_all(str_to_escape, "?", "\\?");
	boost::replace_all(str_to_escape, "/", "\\/");

	return str_to_escape;
}

} /* namespace librevault */
