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

IgnoreList::IgnoreList(FSDirectory& dir, Client& client) : Loggable(client), dir_(dir), client_(client) {
	std::unique_lock<std::mutex> lk(ignore_mtx_);

	// Config paths
	auto ignore_list_its = dir_.dir_options().equal_range("ignore");
	for(auto ignore_list_it = ignore_list_its.first; ignore_list_it != ignore_list_its.second; ignore_list_it++){
		ignored_paths_.insert(ignore_list_it->second.get_value<fs::path>().generic_string());
	}

	// Predefined paths
	ignored_paths_.insert(dir.make_relpath(dir_.block_path()));
	ignored_paths_.insert(dir.make_relpath(dir_.db_path()));
	ignored_paths_.insert(dir.make_relpath(dir_.db_path())+"-journal");
	ignored_paths_.insert(dir.make_relpath(dir_.db_path())+"-wal");
	ignored_paths_.insert(dir.make_relpath(dir_.db_path())+"-shm");
	ignored_paths_.insert(dir.make_relpath(dir_.asm_path()));

	ignored_paths_.erase(std::string());	// If one (or more) of the above returned empty string (aka if one (or more) of the above paths are outside open_path)

	log_->debug() << log_tag() << "IgnoreList initialized";
}
IgnoreList::~IgnoreList() {}

const std::set<std::string>& IgnoreList::ignored_files() const {
	return ignored_paths_;
}

bool IgnoreList::is_ignored(const std::string& relpath) const {
	std::unique_lock<std::mutex> lk(ignore_mtx_);
	return ignored_paths_.find(relpath) != ignored_paths_.end();
}

void IgnoreList::add_ignored(const std::string& relpath){
	std::unique_lock<std::mutex> lk(ignore_mtx_);
	ignored_paths_.insert(relpath);
	log_->debug() << log_tag() << "Added to IgnoreList: " << relpath;
}

void IgnoreList::remove_ignored(const std::string& relpath){
	std::lock_guard<std::mutex> lk(ignore_mtx_);
	ignored_paths_.erase(relpath);
	log_->debug() << log_tag() << "Removed from IgnoreList: " << relpath;
}

std::string IgnoreList::log_tag() const {return dir_.log_tag();}

} /* namespace librevault */
