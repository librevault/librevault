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
#include <boost/asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <thread>

class PeriodicProcess { // TODO: make exception handling
public:
	PeriodicProcess(boost::asio::io_service& io_service, std::function<void()> function) :
		io_service_(io_service), timer_(io_service_), exiting_(false), function_(function) {
		started_handlers_ = 0;
	}
	~PeriodicProcess() {
		wait();
	}

	enum InvokeAfterStrategy {
		RESET_TIMER,
		NO_RESET_TIMER
	};

	void invoke_after(std::chrono::steady_clock::duration duration, InvokeAfterStrategy strategy = RESET_TIMER) {
		if(strategy == NO_RESET_TIMER && timer_.expires_from_now() > std::chrono::nanoseconds(0)) return;

		++started_handlers_;
		timer_.expires_from_now(duration);
		timer_.async_wait([this](const boost::system::error_code& ec){
			if(ec != boost::asio::error::operation_aborted)
				try_concurrent_run();
			--started_handlers_;
		});
	}

	void invoke_post() {
		if(exiting_) return;

		++started_handlers_;
		io_service_.post([this]{
			try_concurrent_run();
			--started_handlers_;
		});
	}

	void wait() {
		exiting_ = true;
		timer_.cancel();
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
	boost::asio::steady_timer timer_;
	std::atomic<unsigned> started_handlers_;
	std::atomic_flag running_;
	std::atomic<bool> exiting_;

	std::function<void()> function_;

	void try_concurrent_run() {
		if(!running_.test_and_set(std::memory_order_acquire)) {
			function_();
			running_.clear(std::memory_order_release);
		}
	}
};