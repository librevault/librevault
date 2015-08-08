/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "AutoIndexer.h"
#include "Indexer.h"
#include "FSDirectory.h"
#include "../../Session.h"
#include "Meta.pb.h"

namespace librevault {

AutoIndexer::AutoIndexer(FSDirectory& dir, Session& session, std::function<void(AbstractDirectory::SignedMeta)> callback) :
		log_(spdlog::get("Librevault")),
		dir_(dir), session_(session),
		monitor_(session_.ios()), index_timer_(session_.ios()) {
	callback_ = callback;

	enqueue_files(index_queue_ = dir.open_storage->pending_files());

	monitor_.add_directory(dir_.open_path().string());
	monitor_operation();
}
AutoIndexer::~AutoIndexer() {}

void AutoIndexer::enqueue_files(const std::string& relpath){
	std::unique_lock<std::mutex> lk(index_queue_m_);
	index_queue_.insert(relpath);
	bump_timer();
}

void AutoIndexer::enqueue_files(const std::set<std::string>& relpath){
	std::unique_lock<std::mutex> lk(index_queue_m_);
	index_queue_.insert(relpath.begin(), relpath.end());
	bump_timer();
}

void AutoIndexer::bump_timer(){
	if(index_timer_.expires_from_now() <= std::chrono::seconds(0)){
		std::chrono::seconds exp_timeout;
		exp_timeout = std::chrono::seconds(dir_.dir_options().get<uint32_t>("index_event_timeout"));

		index_timer_.expires_from_now(exp_timeout);
		index_timer_.async_wait(std::bind(&AutoIndexer::monitor_index, this, std::placeholders::_1));
	}
}

void AutoIndexer::monitor_operation() {
	monitor_.async_monitor([this](boost::system::error_code ec, boost::asio::dir_monitor_event ev){
		if(ec == boost::asio::error::operation_aborted) return;

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
	{
		std::string relpath = dir_.open_storage->make_relpath(ev.path);
		if(!dir_.open_storage->is_skipped(relpath)){
			log_->debug() << "[dir_monitor] " << ev;
			enqueue_files(relpath);
		}
	}
	default: break;
	}
}

void AutoIndexer::monitor_index(const boost::system::error_code& ec) {
	if(ec == boost::asio::error::operation_aborted) return;

	std::set<std::string> index_queue;
	index_queue_m_.lock();
	index_queue.swap(index_queue_);
	index_queue_m_.unlock();

	dir_.indexer->async_index(index_queue, callback_);
}

} /* namespace librevault */
