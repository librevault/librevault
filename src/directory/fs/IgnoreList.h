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
#pragma once
#include "../../pch.h"
#include "../../util/Loggable.h"

namespace librevault {

class Client;
class FSDirectory;
class IgnoreList : protected Loggable {
public:
	IgnoreList(FSDirectory& dir, Client& client);
	virtual ~IgnoreList();

	bool is_ignored(const std::string& relpath) const;

	void add_ignored(const std::string& relpath);
	void remove_ignored(const std::string& relpath);

private:
	FSDirectory& dir_;
	Client& client_;

	static std::regex regex_escape_regex_;
	static std::string regex_escape_replace_;
	static std::string regex_escape(const std::string& str_to_escape);

	mutable std::mutex ignored_paths_mtx_;
	std::map<std::string, std::regex> ignored_paths_;

	std::string log_tag() const;
};

} /* namespace librevault */
