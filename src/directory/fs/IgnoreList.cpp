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
#include "FSDirectory.h"
#include "../../Client.h"

namespace librevault {

IgnoreList::IgnoreList(FSDirectory& dir) : Loggable(dir), dir_(dir) {
	// Config paths
	try {
		auto ignore_list_its = dir_.dir_options().get_child("ignore_paths").equal_range("");
		for(auto ignore_list_it = ignore_list_its.first; ignore_list_it != ignore_list_its.second; ignore_list_it++) {
			add_ignored(ignore_list_it->second.get_value<fs::path>().generic_string());
		}
	}catch(boost::property_tree::ptree_bad_path& e){}

	// Predefined paths
	add_ignored( regex_escape(dir.make_relpath(dir_.block_path())) + R"((\/(.*))?)" );
	add_ignored( regex_escape(dir.make_relpath(dir_.db_path())) );
	add_ignored( regex_escape(dir.make_relpath(dir_.db_path())+"-journal") );
	add_ignored( regex_escape(dir.make_relpath(dir_.db_path())+"-wal") );
	add_ignored( regex_escape(dir.make_relpath(dir_.db_path())+"-shm") );
	add_ignored( regex_escape(dir.make_relpath(dir_.asm_path())) );

	log_->debug() << log_tag() << "IgnoreList initialized";
}

bool IgnoreList::is_ignored(const std::string& relpath) const {
	std::unique_lock<std::mutex> lk(ignored_paths_mtx_);
	for(auto& ignored_path : ignored_paths_) {
		if(std::regex_match(relpath, ignored_path.second)) return true;
	}
	return false;
}

void IgnoreList::add_ignored(const std::string& relpath){
	std::unique_lock<std::mutex> lk(ignored_paths_mtx_);
	if(! relpath.empty()) {
		std::regex relpath_regex(relpath, std::regex::icase | std::regex::optimize | std::regex::collate);
		ignored_paths_.insert({relpath, std::move(relpath_regex)});
		log_->debug() << log_tag() << "Added to IgnoreList: " << relpath;
	}
}

void IgnoreList::remove_ignored(const std::string& relpath){
	std::lock_guard<std::mutex> lk(ignored_paths_mtx_);
	ignored_paths_.erase(relpath);
	log_->debug() << log_tag() << "Removed from IgnoreList: " << relpath;
}

std::regex IgnoreList::regex_escape_regex_ = std::regex("[.^$|()\\[\\]{}*+?\\\\]", std::regex::optimize);
std::string IgnoreList::regex_escape_replace_ = "\\&";

std::string IgnoreList::regex_escape(const std::string& str_to_escape) {
	return std::regex_replace(str_to_escape, regex_escape_regex_, regex_escape_replace_, std::regex_constants::match_default | std::regex_constants::format_sed);
}

} /* namespace librevault */
