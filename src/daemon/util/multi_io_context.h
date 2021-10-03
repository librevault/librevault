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
#pragma once
#include <QString>
#include <boost/asio/io_service.hpp>
#include <thread>

namespace librevault {

class multi_io_context {
 public:
  explicit multi_io_context(QString name);
  ~multi_io_context();

  void start();
  void stop(bool stop_gently = false);

  boost::asio::io_context& ctx() { return ctx_; }

 protected:
  QString name_;
  boost::asio::io_context ctx_;
  std::unique_ptr<boost::asio::io_context::work> ctx_work_;

  std::unique_ptr<std::thread> worker_thread_;

  void run_thread();

  QString log_tag() const { return QString("pool:%1").arg(name_); }
};

}  // namespace librevault
