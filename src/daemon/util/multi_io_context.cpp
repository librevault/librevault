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
