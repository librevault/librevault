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
 */
#include "multi_io_context.h"

#include "log.h"

namespace librevault {

multi_io_context::multi_io_context(QString name) : name_(std::move(name)) {}

multi_io_context::~multi_io_context() { stop(false); }

void multi_io_context::start() {
  ctx_work_ = std::make_unique<boost::asio::io_context::work>(ctx_);
  worker_thread_ = std::make_unique<std::thread>([this] { run_thread(); });  // Running io_context in threads
}

void multi_io_context::stop(bool stop_gently) {
  ctx_work_.reset();

  if (!stop_gently) ctx_.stop();
  if (worker_thread_->joinable()) worker_thread_->join();
}

void multi_io_context::run_thread() {
  LOGD("Asio thread started");
  ctx_.run();
  LOGD("Asio thread started");
}

}  // namespace librevault
