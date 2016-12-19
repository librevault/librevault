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
#include <boost/signals2/signal.hpp>
#include <atomic>
#include <chrono>
#include <thread>

class ScopedTimer { // TODO: make exception handling
public:
	ScopedTimer(boost::asio::io_service& io_service) :
		io_service_(io_service),
		timer_(io_service_),
		shared_tick_(std::make_shared<boost::signals2::signal<void()>>()),
		tick_signal(*shared_tick_) {
	}
	~ScopedTimer() {
		stop();
	}

	void start(std::chrono::steady_clock::duration duration, bool run_immediately, bool reset_timer, bool singleshot) {
		if(!reset_timer && timer_.expires_from_now() > std::chrono::nanoseconds(0)) return;

		if(singleshot)
			shared_tick_->disconnect(1);
		else
			shared_tick_->connect(1, [this]{bump_timer();});

		interval_ = duration;
		auto shared_tick_local = shared_tick_;
		if(run_immediately) {
			io_service_.post([shared_tick_local]{(*shared_tick_local)();});
		}else{
			bump_timer();
		}
	}

	void stop() {
		shared_tick_->disconnect_all_slots();
		timer_.cancel();
	}

private:
	boost::asio::io_service& io_service_;
	boost::asio::steady_timer timer_;
	std::chrono::steady_clock::duration interval_ = std::chrono::steady_clock::duration(0);

	void bump_timer() {
		auto shared_tick_local = shared_tick_;
		timer_.expires_from_now(interval_);
		timer_.async_wait([shared_tick_local](const boost::system::error_code& ec){
			if(ec != boost::asio::error::operation_aborted)
				(*shared_tick_local)();
		});
	}

	std::shared_ptr<boost::signals2::signal<void()>> shared_tick_;

public:
	boost::signals2::signal<void()>& tick_signal;
};
