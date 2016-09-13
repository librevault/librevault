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
#include "IgnoreList.h"
#include "Indexer.h"
#include "FSFolder.h"
#include "../../Client.h"

namespace librevault {

AutoIndexer::AutoIndexer(FSFolder& dir, Client& client) :
		Loggable("AutoIndexer"),
		dir_(dir), client_(client),
		monitor_ios_work_(monitor_ios_),
		monitor_(monitor_ios_),
		rescan_process_(client_.bulk_ios(), [this](PeriodicProcess& process){rescan_operation(process);}),
		index_process_(client_.bulk_ios(), [this](PeriodicProcess& process){perform_index();}) {
	rescan_process_.invoke_post();

	monitor_ios_thread_ = std::thread([&, this](){monitor_ios_.run();});

	monitor_.add_directory(dir_.path().string());
	monitor_operation();
}

AutoIndexer::~AutoIndexer() {
	rescan_process_.wait();
	index_process_.wait();
	monitor_ios_.stop();
	if(monitor_ios_thread_.joinable())
		monitor_ios_thread_.join();
}

void AutoIndexer::enqueue_files(const std::string& relpath) {
	std::unique_lock<std::mutex> lk(index_queue_mtx_);
	index_queue_.insert(relpath);

	index_process_.invoke_after(dir_.params().index_event_timeout, PeriodicProcess::NO_RESET_TIMER);    // Bumps timer
}

void AutoIndexer::enqueue_files(const std::set<std::string>& relpath) {
	std::unique_lock<std::mutex> lk(index_queue_mtx_);
	index_queue_.insert(relpath.begin(), relpath.end());

	index_process_.invoke_after(dir_.params().index_event_timeout, PeriodicProcess::NO_RESET_TIMER);    // Bumps timer
}

void AutoIndexer::prepare_file_assemble(bool with_removal, const std::string& relpath) {
	unsigned skip_events = 2;	// REMOVED, RENAMED (NEW NAME), MODIFIED
	if(with_removal) skip_events++;
	//unsigned skip_events = 0;

	for(unsigned i = 0; i < skip_events; i++)
		prepared_assemble_.insert(relpath);
}

void AutoIndexer::prepare_dir_assemble(bool with_removal, const std::string& relpath) {
	unsigned skip_events = 1;	// ADDED
	skip_events += with_removal ? 1 : 0;

	for(unsigned i = 0; i < skip_events; i++)
		prepared_assemble_.insert(relpath);
}

void AutoIndexer::prepare_deleted_assemble(const std::string& relpath) {
	unsigned skip_events = 1;	// REMOVED or UNKNOWN

	for(unsigned i = 0; i < skip_events; i++)
		prepared_assemble_.insert(relpath);
}

std::set<std::string> AutoIndexer::short_reindex_list() {
	std::set<std::string> file_list;

	// Files present in the file system
	for(auto dir_entry_it = fs::recursive_directory_iterator(dir_.path()); dir_entry_it != fs::recursive_directory_iterator(); dir_entry_it++){
		auto relpath = dir_.normalize_path(dir_entry_it->path());

		if(!dir_.ignore_list->is_ignored(relpath)) file_list.insert(relpath);
	}

	// Prevent incomplete (not assembled, partially-downloaded, whatever) from periodical scans.
	// They can still be indexed by monitor, though.
	for(auto& smeta : dir_.index->get_incomplete_meta()) {
		file_list.erase(smeta.meta().path(dir_.secret()));
	}

	return file_list;
}

std::set<std::string> AutoIndexer::full_reindex_list() {
	std::set<std::string> file_list = short_reindex_list();

	// Files present in index (files added from here will be marked as DELETED)
	for(auto& smeta : dir_.index->get_existing_meta()) {
		file_list.insert(smeta.meta().path(dir_.secret()));
	}

	return file_list;
}

void AutoIndexer::rescan_operation(PeriodicProcess& process) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
	log_->debug() << log_tag() << "Performing full directory rescan";

	if(!dir_.indexer->is_indexing())
		enqueue_files(full_reindex_list());

	process.invoke_after(dir_.params().full_rescan_interval);
}

void AutoIndexer::monitor_operation() {
	log_->trace() << log_tag() << "monitor_operation";
	monitor_.async_monitor([this](boost::system::error_code ec, boost::asio::dir_monitor_event ev){
		log_->trace() << "async_monitor callback ec:" << ec;
		if(ec == boost::asio::error::operation_aborted) {
			log_->debug() << log_tag() << "monitor_operation returned";
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
		std::string relpath = dir_.normalize_path(ev.path);

		auto prepared_assemble_it = prepared_assemble_.find(relpath);
		if(prepared_assemble_it != prepared_assemble_.end()) {
			prepared_assemble_.erase(prepared_assemble_it);
			return;
			// FIXME: "prepares" is a dirty hack. It must be EXTERMINATED!
		}

		if(!dir_.ignore_list->is_ignored(relpath)){
			log_->debug() << "[dir_monitor] " << ev;
			enqueue_files(relpath);
		}
	}
	default: break;
	}
}

void AutoIndexer::perform_index() {
	std::set<std::string> index_queue;
	index_queue_mtx_.lock();
	index_queue.swap(index_queue_);
	index_queue_mtx_.unlock();

	dir_.indexer->async_index(index_queue);
}

} /* namespace librevault */
