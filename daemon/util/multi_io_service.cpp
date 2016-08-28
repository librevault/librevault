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
#include "multi_io_service.h"

namespace librevault {

multi_io_service::multi_io_service(Loggable& parent_loggable, const std::string& name) :
		Loggable(parent_loggable, name) {}

multi_io_service::~multi_io_service() {
	stop();
}

void multi_io_service::start(unsigned thread_count) {
	ios_work_ = std::make_unique<boost::asio::io_service::work>(ios_);

	log_->info() << log_tag() << "Threads: " << thread_count;

	for(unsigned i = 1; i <= thread_count; i++){
		worker_threads_.emplace_back([this, i]{run_thread(i);});	// Running io_service in threads
	}
}

void multi_io_service::stop() {
	ios_work_.reset();
	ios_.stop();

	for(auto& thread : worker_threads_){
		if(thread.joinable()) thread.join();
	}
}

void multi_io_service::run_thread(unsigned worker_number) {
	log_->debug() << log_tag() << "Thread #" << worker_number << " started";
	ios_.run();
	log_->debug() << log_tag() << "Thread #" << worker_number << " stopped";
}

} /* namespace librevault */
