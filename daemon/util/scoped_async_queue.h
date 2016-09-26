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
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <atomic>
#include <thread>

class ScopedAsyncQueue {
public:
	ScopedAsyncQueue(boost::asio::io_service& io_service) : io_service_(io_service), io_service_strand_(io_service) {
		started_handlers_ = 0;
	}
	~ScopedAsyncQueue() {
		wait();
	}

	void invoke_post(std::function<void()> function) {
		++started_handlers_;
		io_service_strand_.post([this, function]{
			function();
			--started_handlers_;
		});
	}

	void wait() {
		if(started_handlers_ != 0) {
			io_service_.dispatch([this] {
				io_service_.poll();
			});
		}
		while(started_handlers_ != 0 && !io_service_.stopped()) {
			std::this_thread::yield();
		}
	}

protected:
	boost::asio::io_service& io_service_;
	boost::asio::io_service::strand io_service_strand_;
	std::atomic<unsigned> started_handlers_;
};