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
#include "pch.h"
#include "Index.h"
#include "folder/fs/chunk/EncStorage.h"
#include "folder/fs/chunk/OpenStorage.h"

#include <dir_monitor/dir_monitor.hpp>
#include <util/periodic_process.h>

namespace librevault {

class AutoIndexer : public Loggable {
public:
	AutoIndexer(FSFolder& dir, Client& client);
	virtual ~AutoIndexer();

	void enqueue_files(const std::string& relpath);
	void enqueue_files(const std::set<std::string>& relpath);

	// A set of VERY DIRTY HACKS
	void prepare_file_assemble(bool with_removal, const std::string& relpath);
	void prepare_dir_assemble(bool with_removal, const std::string& relpath);
	void prepare_deleted_assemble(const std::string& relpath);

	std::set<std::string> short_reindex_list();  // List of files for reindexing on start
	std::set<std::string> full_reindex_list();  // List of files, ready to be reindexed on full reindexing

private:
	FSFolder& dir_;
	Client& client_;

	// Monitor
	io_service monitor_ios_;            // Yes, we have a new thread for each directory, because several dir_monitors on a single io_service behave strangely:
	std::thread monitor_ios_thread_;    // https://github.com/berkus/dir_monitor/issues/42
	io_service::work monitor_ios_work_;
	boost::asio::dir_monitor monitor_;

	std::multiset<std::string> prepared_assemble_;

	// Full rescan operations
	void rescan_operation(PeriodicProcess& process);
	PeriodicProcess rescan_process_;

	// Monitor operations
	void monitor_operation();
	void monitor_handle(const boost::asio::dir_monitor_event& ev);

	// Index queue
	std::set<std::string> index_queue_;
	std::mutex index_queue_mtx_;

	void perform_index();
	PeriodicProcess index_process_;
};

} /* namespace librevault */
