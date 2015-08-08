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
#pragma once
#include "../../pch.h"
#include "../Key.h"
#include "Index.h"
#include "EncStorage.h"
#include "OpenStorage.h"

namespace librevault {

class AutoIndexer {
public:
	AutoIndexer(FSDirectory& dir, Session& session, std::function<void(AbstractDirectory::SignedMeta)> callback);
	virtual ~AutoIndexer();

	void enqueue_files(const std::string& relpath);
	void enqueue_files(const std::set<std::string>& relpath);

private:
	std::shared_ptr<spdlog::logger> log_;
	FSDirectory& dir_;
	Session& session_;

	std::function<void(AbstractDirectory::SignedMeta)> callback_;

	// Monitor
	boost::asio::dir_monitor monitor_;

	void bump_timer();

	// Monitor operations
	void monitor_operation();
	void monitor_handle(const boost::asio::dir_monitor_event& ev);
	std::set<std::string> index_queue_;
	std::mutex index_queue_m_;
	boost::asio::steady_timer index_timer_;
	void monitor_index(const boost::system::error_code& ec);
};

} /* namespace librevault */
