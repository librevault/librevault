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
#include "util/file_util.h"
#include "util/log.h"
#include "util/regex_escape.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>

namespace librevault {

IgnoreNode::IgnoreNode(PathNormalizer& path_normalizer, std::string root) : path_normalizer_(path_normalizer) {
	boost::filesystem::path lvignore_path = path_normalizer_.absolute_path(root) / ".lvignore";
	LOGD("Reading ignore file \"" << lvignore_path.c_str() << "\" for root: \"" << root.c_str() << "\"");

	try {
		file_wrapper lvignore(lvignore_path, "r");
		size_t line_num = 1;
		for(std::string line; std::getline(lvignore.ios(), line); line_num++) {
			auto ignore_regex = parse_ignore_line(line_num, line);
			if(ignore_regex)
				ignore_entries_.push_back(*ignore_regex);
		}
	}catch(std::exception& e){/* no .lvignore */}

	try {
		for(auto it = boost::filesystem::directory_iterator(path_normalizer_.absolute_path(root)); it != boost::filesystem::directory_iterator(); it++) {
			if(it->status().type() == boost::filesystem::directory_file) {
				std::string filename = it->path().filename().generic_string();
				std::string new_root = root.empty() ? filename : root+"/"+filename;
				subnodes_.insert(std::make_pair(filename, std::make_unique<IgnoreNode>(path_normalizer_, new_root)));
			}
		}
	}catch(std::exception& e){
		LOGW("Error reading .lvignore file. e: " << e.what());
	}
}

bool IgnoreNode::is_ignored(const std::string& relpath) const {
	for(auto& ignored_path : ignore_entries_) {
		if(std::regex_match(relpath, ignored_path)) return true;
	}

	auto subnode_it = subnodes_.find(get_first_element(relpath));
	if(subnode_it != subnodes_.end())
		return subnode_it->second->is_ignored(remove_first_element(relpath));

	return false;
}

std::string IgnoreNode::get_first_element(std::string relpath) const {
	relpath.erase(std::find(relpath.begin(), relpath.end(), '/'), relpath.end());
	return relpath;
}

std::string IgnoreNode::remove_first_element(std::string relpath) const {
	auto next_it = std::find(relpath.begin(), relpath.end(), '/');
	if(next_it != relpath.end())
		next_it++;
	relpath.erase(relpath.begin(), next_it);
	return relpath;
}

boost::optional<std::regex> IgnoreNode::parse_ignore_line(size_t line_number, std::string ignore_line) const {
	boost::algorithm::trim_left(ignore_line);
	if(!ignore_line.empty() && ignore_line[0] != '#') {
		std::string ignore_regex = wildcard_to_regex(ignore_line) + R"((?:\/.*)?$)";
		LOGD("Parsed line " << line_number << " as " << "\"" << ignore_regex.c_str() << "\"");
		return std::regex(ignore_regex, std::regex::icase | std::regex::optimize | std::regex::collate);
	}
	return boost::none;
}

IgnoreList::IgnoreList(const FolderParams& params, PathNormalizer& path_normalizer) : path_normalizer_(path_normalizer) {
	ignore_file_tree_created_ = std::chrono::steady_clock::now()-std::chrono::minutes(1);

	ignored_paths_.reserve(params.ignore_paths.size()+1);

	add_ignored(regex_escape(path_normalizer_.normalize_path(params.system_path.toStdString())) + R"((?:\/.*)?$)");
	for(auto path : params.ignore_paths)
		add_ignored(path.toStdString());

	LOGD("IgnoreList initialized");
}

bool IgnoreList::is_ignored(const std::string& relpath) const {
	std::unique_lock<std::mutex> lk(ignored_paths_mtx_);
	for(auto& ignored_path : ignored_paths_) {
		if(std::regex_match(relpath, ignored_path)) return true;
	}

	maybe_renew_ignore_file();
	return ignore_file_tree_->is_ignored(relpath);
}

void IgnoreList::add_ignored(const std::string& relpath) {
	std::unique_lock<std::mutex> lk(ignored_paths_mtx_);
	if(!relpath.empty()) {
		std::regex relpath_regex(relpath, std::regex::icase | std::regex::optimize | std::regex::collate);
		ignored_paths_.push_back(std::move(relpath_regex));
		LOGD("Added to IgnoreList: " << relpath.c_str());
	}
}

void IgnoreList::maybe_renew_ignore_file() const {
	//std::lock_guard<std::mutex> lk(ignored_paths_mtx_);
	if(std::chrono::steady_clock::now() - ignore_file_tree_created_ > std::chrono::seconds(10)) {
		ignore_file_tree_created_ = std::chrono::steady_clock::now();
		ignore_file_tree_ = std::make_unique<IgnoreNode>(path_normalizer_, "");
	}
}

} /* namespace librevault */
