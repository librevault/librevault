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
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>

class ScopedAsyncQueue : public std::enable_shared_from_this<ScopedAsyncQueue> {
public:
	ScopedAsyncQueue(boost::asio::io_service& io_service) : io_service_(io_service), io_service_strand_(io_service) {
		started_handlers_ = 0;
	}
	~ScopedAsyncQueue() {
		stop();
	}

	bool post(std::function<void()> function, bool drop_next = false) {
		std::unique_lock<decltype(queue_mtx_)> lk(queue_mtx_);
		if(drop_next_) return false;

		++started_handlers_;
		io_service_strand_.post([this, function]{
			function();
			--started_handlers_;
		});

		drop_next_ = drop_next;
		return true;
	}

	template <typename ...Args, typename AsyncHandler>
	std::function<void(Args...)> wrap(AsyncHandler handler) {
		std::function<void(Args...)> function = handler;
		try {
			return [queue = shared_from_this(), function](Args... args){
				queue->post([function, args...]{function(args...);});
			};
		}catch(std::bad_weak_ptr& e) {
			return [queue = this, function](Args... args){
				queue->post([function, args...]{function(args...);});
			};
		}
	}

	void stop() {
		std::unique_lock<decltype(queue_mtx_)> lk(queue_mtx_);
		drop_next_ = true;

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
	bool drop_next_ = false;

	std::mutex queue_mtx_;
};
