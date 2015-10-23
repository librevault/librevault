/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "../../pch.h"
#include "../../util/Loggable.h"

namespace librevault {

class FSDirectory;
class IgnoreList : protected Loggable {
public:
	IgnoreList(FSDirectory& dir, Session& session);
	virtual ~IgnoreList();

	const std::set<std::string>& ignored_files() const;
	bool is_ignored(const std::string& relpath) const;

	void add_ignored(const std::string& relpath);
	void remove_ignored(const std::string& relpath);

private:
	FSDirectory& dir_;
	Session& session_;

	std::set<std::string> ignored_paths_;

	std::mutex ignore_mtx_;

	std::string log_tag() const;
};

} /* namespace librevault */
