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
#include <control/Config.h>
#include "util/log_scope.h"
#include <mutex>
#include <regex>

namespace librevault {

class FolderParams;
class PathNormalizer;

class IgnoreNode {
	LOG_SCOPE("IgnoreNode");

public:
	IgnoreNode(PathNormalizer& path_normalizer, std::string root);

	bool is_ignored(const std::string& relpath) const;

private:
	std::map<std::string, std::unique_ptr<IgnoreNode>> subnodes_;
	PathNormalizer& path_normalizer_;
	std::vector<std::regex> ignore_entries_;

	std::string get_first_element(std::string relpath) const;
	std::string remove_first_element(std::string relpath) const;
	boost::optional<std::regex> parse_ignore_line(size_t line_number, std::string ignore_line) const;
};

class IgnoreList {
	LOG_SCOPE("IgnoreList");

public:
	IgnoreList(const FolderParams& params, PathNormalizer& path_normalizer);
	virtual ~IgnoreList() {}

	bool is_ignored(const std::string& relpath) const;

private:
	PathNormalizer& path_normalizer_;

	mutable std::mutex ignored_paths_mtx_;
	std::vector<std::regex> ignored_paths_;

	mutable std::unique_ptr<IgnoreNode> ignore_file_tree_;
	mutable std::chrono::steady_clock::time_point ignore_file_tree_created_;

	// ignore management
	void add_ignored(const std::string& relpath);

	void maybe_renew_ignore_file() const;
};

} /* namespace librevault */
