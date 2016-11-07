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
#include "AutoIndexer.h"
#include "Index.h"
#include "Indexer.h"
#include "control/FolderParams.h"
#include "folder/IgnoreList.h"
#include "util/log.h"

namespace librevault {

AutoIndexer::AutoIndexer(const FolderParams& params, Index& index, Indexer& indexer, IgnoreList& ignore_list, PathNormalizer& path_normalizer, io_service& ios) :
		params_(params),
		index_(index), indexer_(indexer), ignore_list_(ignore_list),
		path_normalizer_(path_normalizer),
		monitor_ios_work_(monitor_ios_),
		monitor_(monitor_ios_),
		rescan_process_(ios, [this](PeriodicProcess& process){rescan_operation(process);}),
		index_process_(ios, [this](PeriodicProcess& process){perform_index();}) {
	rescan_process_.invoke_post();

	monitor_ios_thread_ = std::thread([&, this](){monitor_ios_.run();});

	monitor_.add_directory(params_.path.string());
	monitor_operation();
}

AutoIndexer::~AutoIndexer() {
	rescan_process_.wait();
	index_process_.wait();
	monitor_ios_.stop();
	if(monitor_ios_thread_.joinable())
		monitor_ios_thread_.join();
}

void AutoIndexer::enqueue_files(const std::set<std::string>& relpath) {
	LOGFUNC();
	std::unique_lock<std::mutex> lk(index_queue_mtx_);
	index_queue_.insert(relpath.begin(), relpath.end());

	index_process_.invoke_after(params_.index_event_timeout, PeriodicProcess::NO_RESET_TIMER);    // Bumps timer
}

void AutoIndexer::prepare_assemble(const std::string relpath, Meta::Type type, bool with_removal) {
	unsigned skip_events = 0;
	if(with_removal || type == Meta::Type::DELETED) skip_events++;

	switch(type) {
		case Meta::FILE:
			skip_events += 2;   // RENAMED (NEW NAME), MODIFIED
			break;
		case Meta::DIRECTORY:
			skip_events += 1;   // ADDED
			break;
		default:;
	}

	for(unsigned i = 0; i < skip_events; i++)
		prepared_assemble_.insert(relpath);
}

std::set<std::string> AutoIndexer::reindex_list() {
	std::set<std::string> file_list;

	// Files present in the file system
	for(auto dir_entry_it = fs::recursive_directory_iterator(params_.path); dir_entry_it != fs::recursive_directory_iterator(); dir_entry_it++){
		auto relpath = path_normalizer_.normalize_path(dir_entry_it->path());

		if(!ignore_list_.is_ignored(relpath)) file_list.insert(relpath);
	}

	// Prevent incomplete (not assembled, partially-downloaded, whatever) from periodical scans.
	// They can still be indexed by monitor, though.
	for(auto& smeta : index_.get_incomplete_meta()) {
		file_list.erase(smeta.meta().path(params_.secret));
	}

	// Files present in index (files added from here will be marked as DELETED)
	for(auto& smeta : index_.get_existing_meta()) {
		file_list.insert(smeta.meta().path(params_.secret));
	}

	return file_list;
}

void AutoIndexer::rescan_operation(PeriodicProcess& process) {
	LOGFUNC();
	LOGD("Performing full directory rescan");

	if(!indexer_.is_indexing())
		enqueue_files(reindex_list());

	process.invoke_after(params_.full_rescan_interval);
}

void AutoIndexer::monitor_operation() {
	LOGFUNC();
	monitor_.async_monitor([this](boost::system::error_code ec, boost::asio::dir_monitor_event ev){
		LOGT("async_monitor callback ec:" << ec);
		if(ec == boost::asio::error::operation_aborted) {
			LOGD("monitor_operation returned");
			return;
		}

		monitor_handle(ev);
		monitor_operation();
	});
}

void AutoIndexer::monitor_handle(const boost::asio::dir_monitor_event& ev) {
	switch(ev.type){
	case boost::asio::dir_monitor_event::added:
	case boost::asio::dir_monitor_event::modified:
	case boost::asio::dir_monitor_event::renamed_old_name:
	case boost::asio::dir_monitor_event::renamed_new_name:
	case boost::asio::dir_monitor_event::removed:
	case boost::asio::dir_monitor_event::null:
	{
		std::string relpath = path_normalizer_.normalize_path(ev.path);

		auto prepared_assemble_it = prepared_assemble_.find(relpath);
		if(prepared_assemble_it != prepared_assemble_.end()) {
			prepared_assemble_.erase(prepared_assemble_it);
			return;
			// FIXME: "prepares" is a dirty hack. It must be EXTERMINATED!
		}

		if(!ignore_list_.is_ignored(relpath)){
			LOGD("[dir_monitor] " << ev);
			enqueue_files(relpath);
		}
	}
	default: break;
	}
}

void AutoIndexer::perform_index() {
	LOGFUNC();
	std::set<std::string> index_queue;
	index_queue_mtx_.lock();
	index_queue.swap(index_queue_);
	index_queue_mtx_.unlock();

	indexer_.async_index(index_queue);
}

} /* namespace librevault */
