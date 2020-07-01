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
