/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "src/pch.h"
#include "Index.h"
#include "EncStorage.h"
#include "OpenStorage.h"

#include <dir_monitor/dir_monitor.hpp>

namespace librevault {

class AutoIndexer : public Loggable {
public:
	AutoIndexer(FSFolder& dir, Client& client);
	virtual ~AutoIndexer() {}

	void enqueue_files(const std::string& relpath);
	void enqueue_files(const std::set<std::string>& relpath);

	// A set of VERY DIRTY HACKS
	void prepare_file_assemble(bool with_removal, const std::string& relpath);
	void prepare_dir_assemble(bool with_removal, const std::string& relpath);
	void prepare_deleted_assemble(const std::string& relpath);

private:
	FSFolder& dir_;
	Client& client_;

	// Monitor
	boost::asio::dir_monitor monitor_;

	std::multiset<std::string> prepared_assemble_;

	void bump_timer();

	// Monitor operations
	void monitor_operation();
	void monitor_handle(const boost::asio::dir_monitor_event& ev);
	std::set<std::string> index_queue_;
	std::mutex index_queue_mtx_;
	boost::asio::steady_timer index_timer_;
	void monitor_index(const boost::system::error_code& ec);
};

} /* namespace librevault */
