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

#include "folder/PathNormalizer.h"
#include "util/log_scope.h"
#include "util/network.h"
#include "util/scoped_async_queue.h"
#include "util/scoped_timer.h"
#include <librevault/Meta.h>
#include <dir_monitor/dir_monitor.hpp>
#include <mutex>

namespace librevault {

class Index;
class Indexer;
class IgnoreList;

class AutoIndexer {
	LOG_SCOPE("AutoIndexer");
public:
	AutoIndexer(const FolderParams& params, Index& index, Indexer& indexer, IgnoreList& ignore_list, PathNormalizer& path_normalizer, io_service& ios);
	virtual ~AutoIndexer();

	// A VERY DIRTY HACK
	void prepare_assemble(const std::string relpath, Meta::Type type, bool with_removal = false);

private:
	const FolderParams& params_;
	Index& index_;
	Indexer& indexer_;
	IgnoreList& ignore_list_;
	PathNormalizer& path_normalizer_;

	std::set<std::string> reindex_list();

	// Monitor
	io_service monitor_ios_;            // Yes, we have a new thread for each directory, because several dir_monitors on a single io_service behave strangely:
	std::thread monitor_ios_thread_;    // https://github.com/berkus/dir_monitor/issues/42
	io_service::work monitor_ios_work_;
	boost::asio::dir_monitor monitor_;

	std::multiset<std::string> prepared_assemble_;

	// Full rescan operations
	ScopedAsyncQueue scoped_async_queue_;
	void rescan_operation();
	ScopedTimer rescan_timer_;

	// Monitor operations
	void monitor_operation();
	void monitor_handle(const boost::asio::dir_monitor_event& ev);

	// Index queue
	std::set<std::string> index_queue_;
	std::mutex index_queue_mtx_;
	void enqueue_files(const std::set<std::string>& relpath);
	void enqueue_files(const std::string& relpath) {enqueue_files(std::set<std::string>({relpath}));}

	void perform_index();
	ScopedTimer index_event_timer_;
};

} /* namespace librevault */
